#include "module.hpp"
#include "ast/import.hpp"
#include "ast/walk.hpp"
#include "ast/ast.hpp"
#include "analyze/identresolution.hpp"
#include "analyze/unused.hpp"
#include "transform/flow.hpp"
#include "babel.hpp"
#include "global.hpp"
#include "moduleresolver.hpp"
#include "reporting.hpp"
#include "utils.hpp"
#include <iostream>
#include <json.hpp>
#include <limits>
#include <v8.h>

using json = nlohmann::json;
using namespace std;
namespace fs = filesystem;

Module::Module(IsolateWrapper& isolateWrapper, fs::path path)
    : BasicModule(isolateWrapper)
    , path{ path }
{
    v8::Isolate::Scope isolateScope(isolate);
    v8::HandleScope handleScope(isolate);
    v8::Local<v8::String> filename = v8::String::NewFromUtf8(isolate, path.c_str());
    v8::Local<v8::Context> context = prepareGlobalContext(isolateWrapper);
    v8::Local<v8::Function> localRequire = v8::Function::New(context, ModuleResolver::requireFunction, filename).ToLocalChecked();
    context->Global()->Set(context, v8::String::NewFromUtf8(isolate, "require"), localRequire).ToChecked();
    context->SetEmbedderData((int)EmbedderDataIndex::ModulePath, filename);
    persistentContext.Reset(isolate, context);

    originalSource = readFileStr(path.c_str());
    //std::tie(transpiledSource, jast) = transpileScript(isolateWrapper, originalSource);
    jast = transpileScript(isolateWrapper, originalSource);
    ast = importBabylonAst(*this, jast);
    transpiledSource = stripFlowTypes(originalSource, *ast);
}

const AstRoot& Module::getAst()
{
    return *ast;
}

void Module::analyze()
{
    resolveLocalIdentifiers();
    resolveLocalXRefs();
    resolveImportedIdentifiers();

    findUnusedLocalDeclarations(*this);

    // A good analysis strategy might be to start with the symbols we have in our local module,
    // and move to imports whenever they are actually used by our local code.
    //
    // This means we'll know a real use/call site for almost every symbol we analyze,
    // We'll also be able to mark any analyzed symbols, and whatever's left at the end is apparently unused (warn about it)
}

void Module::resolveLocalIdentifiers()
{
    using namespace v8;

    if (localIdentifierResolutionDone)
        return;
    localIdentifierResolutionDone = true;

    trace("Resolving local identifiers for module "+path.string());
    Isolate::Scope isolateScope(isolate);
    HandleScope handleScope(isolate);
    Local<Context> context = persistentContext.Get(isolate);
    Context::Scope contextScope(context);
    Local<v8::Module> module = getExecutableModule();

    IdentifierResolutionResult result = resolveModuleIdentifiers(context, ast);
    resolvedLocalIdentifiers = result.resolvedIdentifiers;
    missingContextIdentifiers = result.missingGlobalIdentifiers;

    // Any missing identifier in one of our imports is also considered missing in our module,
    // because imports are evaluated by v8 in the current module's context,
    // so we need to have any missing global symbols of our imports in our own context.
    for (int i=0; i<module->GetModuleRequestsLength(); ++i) {
        auto importName = module->GetModuleRequest(i);
        v8::String::Utf8Value importNameStr(isolate, importName);
        if (NativeModule::hasModule(*importNameStr))
            continue;
        Module& importedModule = reinterpret_cast<Module&>(ModuleResolver::getModule(*this, *importNameStr, true));
        missingContextIdentifiers.insert(missingContextIdentifiers.end(), importedModule.missingContextIdentifiers.begin(), importedModule.missingContextIdentifiers.end());
    }
}

void Module::resolveLocalXRefs()
{
    if (localXRefsDone)
        return;
    localXRefsDone = true;

    for (auto idToDecl : resolvedLocalIdentifiers) {
        auto& usages = localXRefs[idToDecl.second];
        usages.push_back(idToDecl.first);
    }
}

void Module::resolveImportedIdentifiers()
{
    using namespace v8;

    if (importedIdentifierResolutionDone)
        return;
    importedIdentifierResolutionDone = true;

    // TODO: It's time to take advantage of the import & module system we painstakingly build!
    // Let's resolve cross-file references.
    //
    // Make a function that takes an ES6 ImportSpecifier and returns the declaration of the exported identifier in the imported module
    // And we don't want to return the node at the exported identifier, but the declaration of the local identifier in the imported module

    //trace("Resolving imported identifiers for module "+path.string());

}

v8::Local<v8::Module> Module::compileModuleFromSource(const string& filename, const string& source)
{
    using namespace v8;

    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);

    TryCatch trycatch(isolate);
    Local<Context> context = persistentContext.Get(isolate);
    Context::Scope contextScope(context);
    Local<String> filenameStr = String::NewFromUtf8(isolate, filename.c_str());
    Local<String> sourceStr = String::NewFromUtf8(isolate, source.c_str());

    ScriptOrigin origin(filenameStr,
        Integer::New(isolate, 0),
        Integer::New(isolate, 0),
        False(isolate),
        Local<Integer>(),
        Local<Value>(),
        False(isolate),
        False(isolate),
        True(isolate));

    ScriptCompiler::Source moduleSource(sourceStr, origin);
    Local<v8::Module> module;
    if (!ScriptCompiler::CompileModule(isolate, &moduleSource).ToLocal(&module)) {
        reportV8Exception(isolate, &trycatch);
        throw runtime_error("Failed to compile module");
    }
    return module;
}

v8::Local<v8::Module> Module::getCompiledModule()
{
    using namespace v8;

    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);

    if (compiledModule.IsEmpty()) {
        auto module = compileModuleFromSource(path, transpiledSource);
        compiledModule.Reset(isolate, module);
    }
    return handleScope.Escape(compiledModule.Get(isolate));
}

v8::Local<v8::Module> Module::getExecutableModule()
{
    using namespace v8;

    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);
    Local<v8::Module> compiledModule = getCompiledModule();

    // This is necessary before the module is ready to be run!
    resolveLocalIdentifiers();

    return handleScope.Escape(compiledModule);
}

v8::Local<v8::Module> Module::getExecutableES6Module()
{
    using namespace v8;

    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);

    auto module = getExecutableModule();
    if (isES6Module())
        return module;

    if (compiledThunkModule.IsEmpty()) {
        Local<Context> context = isolate->GetCurrentContext();
        Context::Scope contextScope(context);

        auto thunkSource = string("let _module = require('")+path.c_str()+"'); export default _module;";

        auto exports = getExports();
        auto exportedProps = exports->GetOwnPropertyNames(context).ToLocalChecked();
        //cout << "Got "<<exportedProps->Length()<<" properties"<<endl;
        for (uint32_t i=0; i<exportedProps->Length(); ++i) {
            auto nameStr = std::string(*v8::String::Utf8Value(isolate, exportedProps->Get(i).As<String>()));
            //cout <<nameStr<<endl;

            thunkSource += "export let "+nameStr+"=_module."+nameStr+";";
        }
        //cout << thunkSource << endl;

        compiledThunkModule.Reset(isolate, compileModuleFromSource(path, thunkSource));
    }
    return handleScope.Escape(compiledThunkModule.Get(isolate));
}

int Module::getCompiledModuleIdentityHash()
{
    return getCompiledModule()->GetIdentityHash();
}

string Module::getPath() const
{
    return path;
}

std::unordered_map<Identifier *, std::vector<Identifier *> > Module::getLocalXRefs()
{
    resolveLocalXRefs();
    return localXRefs;
}

std::unordered_map<Identifier *, Identifier *> Module::getResolvedLocalIdentifiers()
{
    resolveLocalIdentifiers();
    return resolvedLocalIdentifiers;
}

void Module::evaluate()
{
    using namespace v8;

    trace("Evaluating module "+path.string());

    Isolate::Scope isolateScope(isolate);
    HandleScope handleScope(isolate);
    TryCatch trycatch(isolate);
    Local<Context> context = persistentContext.Get(isolate);
    Context::Scope contextScope(context);
    Local<v8::Module> module = getExecutableModule();
    defineMissingGlobalIdentifiers(context, missingContextIdentifiers);

    if (auto maybeBool = module->InstantiateModule(context, ModuleResolver::resolveImportCallback); maybeBool.IsNothing() || !maybeBool.ToChecked()) {
        reportV8Exception(isolate, &trycatch);
        throw runtime_error("Failed to compile module");
    }

    auto result = module->Evaluate(context);
    (void)result;
    if (module->GetStatus() == v8::Module::kErrored) {
        Local<v8::Object> except = module->GetException().As<Object>();
        v8::String::Utf8Value exceptStackStr(isolate, except->Get(String::NewFromUtf8(isolate, "stack")));

        throw std::runtime_error(std::string("Error when evaluating module '")+path.c_str()+"': "+*exceptStackStr);
    }
}

bool Module::isES6Module()
{
    /// TODO: (later) This function should also look for dynamic ES6 imports in the AST, not just top-level.
    /// Thankfully that's super rare to have dynamic imports and no other top level imports and exports at this point in time
    for (AstNode* node : ast->getBody()) {
        if (node->getType() == AstNodeType::Import
            || node->getType() == AstNodeType::ImportDeclaration
            || node->getType() == AstNodeType::ImportDefaultSpecifier
            || node->getType() == AstNodeType::ImportNamespaceSpecifier
            || node->getType() == AstNodeType::ImportSpecifier
            || node->getType() == AstNodeType::ExportAllDeclaration
            || node->getType() == AstNodeType::ExportDefaultDeclaration
            || node->getType() == AstNodeType::ExportNamedDeclaration
            || node->getType() == AstNodeType::ExportSpecifier
            || node->getType() == AstNodeType::ExportDefaultSpecifier)
            return true;
    }
    return false;
}

void Module::resolveProjectImports(std::filesystem::path projectDir)
{
    using namespace v8;

    if (importsResolved)
        return;
    importsResolved = true;

    trace("Resolving imports of module "+path.string());

    Isolate::Scope isolateScope(isolate);
    HandleScope handleScope(isolate);

    // Resolve ES6 imports
    auto module = getCompiledModule();
    for (int i=0; i<module->GetModuleRequestsLength(); ++i) {
        auto importName = module->GetModuleRequest(i);
        String::Utf8Value importNameStr(isolate, importName);
        if (NativeModule::hasModule(*importNameStr))
            continue;
        Module& importedModule = reinterpret_cast<Module&>(ModuleResolver::getModule(*this, *importNameStr, true));
        if (ModuleResolver::isProjectModule(projectDir, importedModule.path))
            importedModule.resolveProjectImports(projectDir);
    }

    // Resolve imports through require() calls
    // TODO: We only resolve require calls taking a literal now, we should try to get possibly values if it takes an identifier!
    // For example if there's an if/else assigning a variable, and we import that variable, at some point we should aim to resolve that!
    walkAst(*ast, [&](AstNode& node){
        auto parent = (CallExpression*)node.getParent();
        const auto& args = parent->getArguments();
        if (args.size() < 1 || args[0]->getType() != AstNodeType::StringLiteral)
            return;
        auto arg = ((StringLiteral*)args[0])->getValue();

        // TODO: v8 gives parse errors if we import a .json directly, we need to autogenerate a wrapper of some sort for json modules
        // (This shows again that JSON is not JS!)
        if (fs::path(arg).extension() == ".json")
            return;

        if (NativeModule::hasModule(arg))
            return;
        try {
            Module& importedModule = reinterpret_cast<Module&>(ModuleResolver::getModule(*this, arg, false));
            if (ModuleResolver::isProjectModule(projectDir, importedModule.path))
                importedModule.resolveProjectImports(projectDir);
        } catch (std::runtime_error&) {
            // This is fine. We're trying to resolve every require() everywhere,
            // not just those reachable from the global scope, so some are expected to fail...
        }
    }, [&](AstNode& node) {
        if (node.getType() != AstNodeType::Identifier
                || node.getParent()->getType() != AstNodeType::CallExpression)
            return false;
        auto& id = (Identifier&)node;
        return id.getName() == "require" && resolvedLocalIdentifiers.find(&id) == resolvedLocalIdentifiers.end();
    });
}
