#include "nativemodule.hpp"
#include "isolatewrapper.hpp"
#include "module/native/modules.hpp"
#include "utils.hpp"

#include <v8.h>

using namespace v8;

NativeModule::NativeModule(IsolateWrapper& isolateWrapper, std::string name)
    : BasicModule(isolateWrapper)
    , name{ name }
{
    auto it = nativeModulesTemplates.find(name);
    if (it == nativeModulesTemplates.end())
        throw std::runtime_error("Trying to instantiate native module that doesn't exist: " + name);
    moduleTemplate = it->second;

    Local<ObjectTemplate> globalTemplate = ObjectTemplate::New(isolate);
    Local<Context> context = Context::New(isolate, {}, globalTemplate);
    persistentContext.Reset(isolate, context);
}

std::string NativeModule::getPath() const
{
    return "<builtin>";
}

v8::MaybeLocal<Module> NativeModule::getWrapperModule()
{
    v8::Isolate::Scope isolateScope(isolate);
    v8::HandleScope handleScope(isolate);

    TryCatch trycatch(isolate);

    auto wrapperSource = std::string("export default require('") + name + "');";
    Local<String> filename = String::NewFromUtf8(isolate, ("<builtin "+name+">").c_str());
    Local<String> localSource = String::NewFromUtf8(isolate, wrapperSource.c_str());
    ScriptOrigin origin(filename,
        Integer::New(isolate, 0),
        Integer::New(isolate, 0),
        False(isolate),
        Local<Integer>(),
        Local<Value>(),
        False(isolate),
        False(isolate),
        True(isolate));

    ScriptCompiler::Source moduleSource(localSource, origin);
    Local<v8::Module> module;
    if (!ScriptCompiler::CompileModule(isolate, &moduleSource).ToLocal(&module)) {
        reportV8Exception(isolate, &trycatch);
        throw std::runtime_error("Failed to compile module");
    }
    return module;
}

std::vector<std::string> NativeModule::getNativeModuleNames()
{
    std::vector<std::string> names;
    for (auto&& pair : nativeModulesTemplates)
        names.push_back(pair.first);
    return names;
}

bool NativeModule::hasModule(const std::string& name)
{
    return nativeModulesTemplates.find(name) != nativeModulesTemplates.end();
}

void NativeModule::evaluate()
{
    v8::Isolate::Scope isolateScope(isolate);
    v8::HandleScope handleScope(isolate);

    auto context = persistentContext.Get(isolate);
    Context::Scope contextScope(context);

    Local<Object> moduleObj = Object::New(isolate);
    Local<Value> exportsObj = moduleTemplate ? moduleTemplate(isolateWrapper) : Object::New(isolate);
    moduleObj->Set(String::NewFromUtf8(isolate, "exports"), exportsObj);
    context->Global()->Set(context, String::NewFromUtf8(isolate, "module"), moduleObj).ToChecked();
}
