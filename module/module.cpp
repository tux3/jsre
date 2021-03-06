#include "module.hpp"
#include "ast/ast.hpp"
#include "ast/parse.hpp"
#include "ast/walk.hpp"
#include "analyze/identresolution.hpp"
#include "analyze/unused.hpp"
#include "analyze/conditionals.hpp"
#include "analyze/typecheck.hpp"
#include "passes/function/list.hpp"
#include "graph/graph.hpp"
#include "graph/graphbuilder.hpp"
#include "transform/flow.hpp"
#include "global.hpp"
#include "moduleresolver.hpp"
#include "utils/reporting.hpp"
#include "utils/utils.hpp"
#include <limits>
#include <cassert>
#include <v8.h>

using namespace std;
namespace fs = filesystem;

Module::Module(IsolateWrapper& isolateWrapper, fs::path path)
    : BasicModule(isolateWrapper)
    , path{ path }
{
    originalSource = readFileStr(path.c_str());
    astFuture = parseSourceScriptAsync(*this, originalSource);

    v8::Isolate::Scope isolateScope(isolate);
    v8::HandleScope handleScope(isolate);
    v8::Local<v8::String> filename = v8::String::NewFromUtf8(isolate, path.c_str());
    v8::Local<v8::Context> context = prepareGlobalContext(isolateWrapper);
    v8::Local<v8::Function> localRequire = v8::Function::New(context, ModuleResolver::requireFunction, filename).ToLocalChecked();
    context->Global()->Set(context, v8::String::NewFromUtf8(isolate, "require"), localRequire).ToChecked();
    context->SetEmbedderData((int)EmbedderDataIndex::ModulePath, filename);
    persistentContext.Reset(isolate, context);
}

AstRoot& Module::getAst()
{
    if (!ast) {
        assert(astFuture.valid());
        ast.reset(astFuture.get());
    }

    return *ast;
}

void Module::analyze()
{
    resolveLocalIdentifiers();
    resolveLocalXRefs();
    resolveImportedIdentifiers();
    runTypechecks(*this);

    for (const auto& funGraph : functionGraphs) {
        if (!funGraph.second)
            continue;
        for (auto pass : functionPassList)
            pass(*this, *funGraph.second);
    }

    findUnusedLocalDeclarations(*this);
    analyzeConditionals(*this);
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

    IdentifierResolutionResult result = resolveModuleIdentifiers(context, getAst());
    resolvedLocalIdentifiers = move(result.resolvedIdentifiers);
    missingContextIdentifiers = move(result.missingGlobalIdentifiers);
    scopeChain = move(result.scopeChain);

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

    for (auto idToDecl : getResolvedLocalIdentifiers()) {
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

    walkAst(getAst(), [&](AstNode& node){
        resolveImportedIdentifierDeclaration(node);
    }, [&](AstNode& node) {
        if (node.getType() == AstNodeType::ImportDeclaration)
            return WalkDecision::SkipInto;
        else if (node.getType() == AstNodeType::ImportSpecifier || node.getType() == AstNodeType::ImportDefaultSpecifier)
            return WalkDecision::WalkInto;
        else
            return WalkDecision::SkipOver;
    });
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
    return handleScope.Escape(module);
}

v8::Local<v8::Module> Module::getCompiledModule()
{
    using namespace v8;

    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);

    if (compiledModule.IsEmpty()) {
        auto transpiledSource = stripFlowTypes(originalSource, getAst());
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
        Local<Context> context = persistentContext.Get(isolate);
        Context::Scope contextScope(context);

        auto thunkSource = "const _m=require('"s+path.c_str()+"');export default _m;\n";
        string exportsStr;

        auto exports = getExports();
        auto exportedProps = exports->GetOwnPropertyNames(context).ToLocalChecked();
        //cout << "Got "<<exportedProps->Length()<<" properties"<<endl;
        for (uint32_t i=0; i<exportedProps->Length(); ++i) {
            auto nameStr = std::string(*v8::String::Utf8Value(isolate, exportedProps->Get(i).As<String>()));
            //cout <<nameStr<<endl;

            string tmpName = "_"+to_string(i);
            thunkSource += "const "+tmpName+"=_m."+nameStr+";";
            exportsStr += tmpName +" as "+nameStr+",";
        }
        thunkSource += "\nexport {"+exportsStr+"};";
        //cout << thunkSource << endl;

        compiledThunkModule.Reset(isolate, compileModuleFromSource(path, thunkSource));
    }
    return handleScope.Escape(compiledThunkModule.Get(isolate));
}

Graph *Module::getFunctionGraph(Function &fun)
{
    auto it = functionGraphs.find(&fun);
    if (it != functionGraphs.end())
        return it->second.get();

    GraphBuilder builder(fun);
    try {
        const auto& result = functionGraphs.insert({&fun, builder.buildFromAst()});
        return result.first->second.get();
    } catch (const runtime_error& e) {
        trace(fun, "Failed to build function graph: "s+e.what());
        functionGraphs.insert({&fun, nullptr});
        return nullptr;
    }
}

shared_ptr<ClassTypeInfo> Module::getClassExtraTypeInfo(Class &c)
{
    auto it = classExtraTypeInfos.find(&c);
    if (it != classExtraTypeInfos.end())
        return it->second;

    return classExtraTypeInfos[&c] = make_shared<ClassTypeInfo>(c);
}

int Module::getCompiledModuleIdentityHash()
{
    return getCompiledModule()->GetIdentityHash();
}

const string &Module::getOriginalSource() const
{
    return originalSource;
}

string Module::getPath() const
{
    return path;
}

const std::unordered_map<Identifier *, std::vector<Identifier *> > &Module::getLocalXRefs()
{
    resolveLocalXRefs();
    return localXRefs;
}

const std::unordered_map<Identifier *, Identifier *>& Module::getResolvedLocalIdentifiers()
{
    resolveLocalIdentifiers();
    return resolvedLocalIdentifiers;
}

const LexicalBindings& Module::getScopeChain()
{
    resolveLocalIdentifiers();
    return *scopeChain;
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

    if (auto maybeBool = module->InstantiateModule(context, ModuleResolver::getResolveImportCallback(*this)); maybeBool.IsNothing() || !maybeBool.ToChecked()) {
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
    for (AstNode* node : getAst().getBody()) {
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

void Module::resolveProjectImports(const fs::path& projectDir)
{
    using namespace v8;

    if (importsResolved)
        return;
    importsResolved = true;

    trace("Resolving imports of module "+path.string());

    Isolate::Scope isolateScope(isolate);
    HandleScope handleScope(isolate);

    vector<Module*> modulesToResolve;

    // Resolve ES6 imports
    auto module = getCompiledModule();
    for (int i=0; i<module->GetModuleRequestsLength(); ++i) {
        auto importName = module->GetModuleRequest(i);
        String::Utf8Value importNameStr(isolate, importName);
        if (NativeModule::hasModule(*importNameStr))
            continue;
        if (ModuleResolver::isProjectModule(projectDir, getPath(), *importNameStr)) {
            Module& importedModule = reinterpret_cast<Module&>(ModuleResolver::getModule(*this, *importNameStr, false));
            modulesToResolve.push_back(&importedModule);
        }
    }

    // Resolve imports through require() calls
    // TODO: We only resolve require calls taking a literal now, we should try to get possibly values if it takes an identifier!
    // For example if there's an if/else assigning a variable, and we import that variable, at some point we should aim to resolve that!
    walkAst(getAst(), [&](AstNode& node){
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
            if (ModuleResolver::isProjectModule(projectDir, getPath(), arg)) {
                Module& importedModule = reinterpret_cast<Module&>(ModuleResolver::getModule(*this, arg, false));
                modulesToResolve.push_back(&importedModule);
            }
        } catch (std::runtime_error&) {
            // This is fine. We're trying to resolve every require() everywhere,
            // not just those reachable from the global scope, so some are expected to fail...
        }
    }, [&](AstNode& node) {
        if (node.getType() != AstNodeType::Identifier
                || node.getParent()->getType() != AstNodeType::CallExpression)
            return WalkDecision::SkipInto;
        auto& id = (Identifier&)node;
        if (id.getName() == "require" && resolvedLocalIdentifiers.find(&id) == resolvedLocalIdentifiers.end())
            return WalkDecision::WalkInto;
        return WalkDecision::SkipInto;
    });

    for (auto module : modulesToResolve)
        module->resolveProjectImports(projectDir);
}
