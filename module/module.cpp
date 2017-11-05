#include "module.hpp"
#include "ast/import.hpp"
#include "ast/walk.hpp"
#include "ast/ast.hpp"
#include "babel.hpp"
#include "global.hpp"
#include "moduleresolver.hpp"
#include "utils.hpp"
#include <iostream>
#include <json.hpp>
#include <limits>
#include <v8.h>

using json = nlohmann::json;
using namespace std;
namespace fs = experimental::filesystem;

Module::Module(IsolateWrapper& isolateWrapper, fs::path path)
    : BasicModule(isolateWrapper)
    , path{ path }
{
    v8::Isolate::Scope isolateScope(isolate);
    v8::HandleScope handleScope(isolate);
    v8::Local<v8::String> filename = v8::String::NewFromUtf8(isolate, path.c_str());
    v8::Local<v8::Context> context = prepareGlobalContext(isolateWrapper);
    context->SetEmbedderData((int)EmbedderDataIndex::ModulePath, filename);
    persistentContext.Reset(isolate, context);

    originalSource = readFileStr(path.c_str());
    std::tie(transpiledSource, jast) = transpileScript(isolateWrapper, originalSource);
    ast = importBabylonAst(jast);
}

const AstRoot& Module::getAst()
{
    return *ast;
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

v8::Local<v8::Module> Module::getCompiledES6Module()
{
    using namespace v8;

    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);

    auto module = getCompiledModule();
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

string Module::getPath() const
{
    return path;
}

void Module::reconstructTypes()
{
    cout << "Propagating type information for module " << path << endl;

    /// TODO: (later, once imports work!) Traverse the AST completely, building an SSA representation as we go,
    /// and linking each Identifier in the AST to its counterpart in the SSA code
    /// After this we can start assigning and propagating types to the SSA variables.

    json programAst = jast["program"];
    walkAst(programAst, [](json& node) {
        cout << node["type"];
        return true;
    });
}

void Module::resolveExports()
{
    using namespace v8;

    cout << "Resolving exports for module " << path << endl;

    Isolate::Scope isolateScope(isolate);
    HandleScope handleScope(isolate);
    TryCatch trycatch(isolate);
    Local<Context> context = persistentContext.Get(isolate);
    Context::Scope contextScope(context);

    Local<String> filename = String::NewFromUtf8(isolate, path.c_str());
    Local<v8::Function> localRequire = v8::Function::New(context, ModuleResolver::requireFunction, filename).ToLocalChecked();
    context->Global()->Set(context, String::NewFromUtf8(isolate, "require"), localRequire).ToChecked();

    Local<v8::Module> module = getCompiledModule();
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
            || node->getType() == AstNodeType::ExportSpecifier)
            return true;
    }
    return false;
}

std::string Module::resolveImports(v8::Local<v8::Context>& context)
{
    v8::Context::Scope contextScope(context);
    v8::TryCatch trycatch(context->GetIsolate());

    std::string resolvedSource = originalSource;
    size_t offset = 0;
    const json& bodyAst = jast["program"]["body"];
    for (const json& statement : bodyAst) {
        if (statement["type"] != "ImportDeclaration")
            continue;
        size_t start = statement["start"], size = (size_t)statement["end"] - start + 1;
        resolvedSource.erase(start - offset, size);
        offset += size;

        BasicModule& importedModule = ModuleResolver::getModule(static_cast<BasicModule&>(*this), statement["source"]["value"]);
        importedModule.getExports();
    }

    return resolvedSource;
}
