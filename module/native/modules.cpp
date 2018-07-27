#include "module/native/modules.hpp"
#include "isolatewrapper.hpp"
#include "utils.hpp"
#include <v8.h>

#define X(MODULE) \
    { #MODULE, MODULE##ModuleTemplate },

std::unordered_map<std::string, NativeModuleTemplate> nativeModulesTemplates{
    // NOTE: If you're going to define a stub module, remove it from this list first or it'll conflict silently!
    { "child_process", {} },
    { "constants", {} },
    { "http", {} },
    { "http2", {} },
    { "https", {} },
    { "net", {} },
    { "os", {} },
    { "path", {} },
    { "stream", {} },
    { "tls", {} },
    { "vm", {} },
    { "zlib", {} },
    MODULES_X_LIST
};

#undef X

v8::Local<v8::Object> compileNativeModuleScript(IsolateWrapper& isolateWrapper, const std::string& scriptSource)
{
    using namespace v8;

    Isolate* isolate = isolateWrapper.get();
    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);
    Local<Context> context = Context::New(isolate);
    Context::Scope contextScope(context);
    TryCatch trycatch(isolate);

    Local<String> scriptSourceStr = String::NewFromUtf8(isolate, scriptSource.data(),
        NewStringType::kNormal, scriptSource.size())
                                        .ToLocalChecked();

    Local<Script> script;
    if (!Script::Compile(context, scriptSourceStr).ToLocal(&script)) {
        reportV8Exception(isolate, &trycatch);
        throw std::runtime_error("compileNativeModuleScript: Error compiling script");
    }

    Local<Value> result;
    if (!script->Run(context).ToLocal(&result) || !result->IsObject()) {
        reportV8Exception(isolate, &trycatch);
        throw std::runtime_error("compileNativeModuleScript: Error executing script");
    }

    return handleScope.Escape(result.As<Object>());
}

void exportNativeModuleCompiledFunction(v8::Local<v8::Object>& exports, v8::Local<v8::Object>& compiledScript, const char* functioName)
{
    using namespace v8;

    Isolate* isolate = exports->GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    Local<String> name = String::NewFromUtf8(isolate, functioName);
    exports->Set(context, name, compiledScript->Get(context, name).ToLocalChecked()).ToChecked();
}

void exportNativeModuleTrapFunction(v8::Local<v8::Object>& exports, const char* functioName)
{
    using namespace v8;

    Isolate* isolate = exports->GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    Local<String> name = String::NewFromUtf8(isolate, functioName);
    exports->Set(context, name, Function::New(context, nativeModuleTrapFunction, name).ToLocalChecked()).ToChecked();
}

void nativeModuleTrapFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    using namespace v8;
    Local<String> functionNameLocal = args.Data().As<String>();
    std::string functionName(*String::Utf8Value(args.GetIsolate(), functionNameLocal));
    throw std::runtime_error("A script tried to call an unauthorized Node.js function ("
        + functionName
        + ") at global scope.\n"
          "To prevent side-effects during imports and for security reasons, some Node.js functions are not provided.\n"
          "If this function has no side-effects, it may simply be unimplemented at the moment.");
}

void exportNativeModuleStubFunction(v8::Local<v8::Object>& exports, const char* functioName)
{
    using namespace v8;

    Isolate* isolate = exports->GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    Local<String> name = String::NewFromUtf8(isolate, functioName);
    exports->Set(context, name, Function::New(context, nativeModuleStubFunction, name).ToLocalChecked()).ToChecked();
}

void nativeModuleStubFunction(const v8::FunctionCallbackInfo<v8::Value>&)
{
    // Nothing to be done!
}
