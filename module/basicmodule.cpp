#include "basicmodule.hpp"
#include "isolatewrapper.hpp"

using namespace v8;

BasicModule::BasicModule(IsolateWrapper& isolateWrapper)
    : isolateWrapper{ isolateWrapper }
    , isolate{ *isolateWrapper }
    , exportsResolveStarted{ false }
{
    Isolate::Scope isolateScope(isolate);
    HandleScope handleScope(isolate);
}

const v8::Local<v8::Object> BasicModule::getExports()
{
    if (!exportsResolveStarted) {
        exportsResolveStarted = true;
        resolveExports();
    }

    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);

    auto context = persistentContext.Get(isolate);
    Context::Scope contextScope(context);
    Local<Object> moduleObj = context->Global()->Get(String::NewFromUtf8(isolate, "module")).As<v8::Object>();
    Local<Object> exportsObj = moduleObj->Get(String::NewFromUtf8(isolate, "exports")).As<v8::Object>();

    return handleScope.Escape(exportsObj);
}

IsolateWrapper& BasicModule::getIsolateWrapper() const
{
    return isolateWrapper;
}
