#include "v8/isolatewrapper.hpp"
#include "module/global.hpp"
#include "utils/utils.hpp"
#include "v8.hpp"

using namespace v8;

IsolateWrapper::IsolateWrapper()
    : isolate{ Isolate::New(::V8::getInstance().getCreateParams()) }
{
    HandleScope handleScope(isolate);
    Local<Context> localContext = Context::New(isolate);
    localContext->Enter();
    defaultContext.Reset(isolate, localContext);
}

IsolateWrapper::~IsolateWrapper()
{
    {
        HandleScope handleScope(isolate);
        defaultContext.Get(isolate)->Exit();
    }
    isolate->Dispose();
}

v8::Isolate* IsolateWrapper::get()
{
    return isolate;
}
