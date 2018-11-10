#include "v8/isolatewrapper.hpp"
#include "module/native/modules.hpp"
#include <iostream>
#include <v8.h>

using namespace std;
using namespace v8;

void logFunction(const FunctionCallbackInfo<Value>& args)
{
    Local<String> logTypeLocal = args.Data().As<String>();
    std::string logType(*String::Utf8Value(args.GetIsolate(), logTypeLocal));
    cout << "[console." << logType << "] ";

    for (int i = 0; i < args.Length(); ++i) {
        Local<Value> v = args[i];
        if (v->IsString()) {
            String::Utf8Value str(args.GetIsolate(), v.As<String>());
            cout << *str;
        } else {
            String::Utf8Value str(args.GetIsolate(), v->ToString(args.GetIsolate()));
            cout << *str;
        }
        if (i == args.Length() - 1)
            cout << endl;
        else
            cout << ' ';
    }
}

Local<Object> consoleModuleTemplate(IsolateWrapper& isolateWrapper)
{
    Isolate* isolate = *isolateWrapper;
    EscapableHandleScope handleScope(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    Local<Object> exports = Object::New(isolate);

    auto infoStr = String::NewFromUtf8(isolate, "info");
    auto logStr = String::NewFromUtf8(isolate, "log");
    auto warnStr = String::NewFromUtf8(isolate, "warn");
    auto errorStr = String::NewFromUtf8(isolate, "error");
    exports->Set(infoStr, Function::New(context, logFunction, infoStr).ToLocalChecked());
    exports->Set(logStr, Function::New(context, logFunction, logStr).ToLocalChecked());
    exports->Set(warnStr, Function::New(context, logFunction, warnStr).ToLocalChecked());
    exports->Set(errorStr, Function::New(context, logFunction, errorStr).ToLocalChecked());

    return handleScope.Escape(exports);
}
