#include "isolatewrapper.hpp"
#include "module/native/modules.hpp"
#include <iostream>
#include <v8.h>

using namespace std;
using namespace v8;

void logFunction(const FunctionCallbackInfo<Value>& args)
{
    Local<String> logTypeLocal = args.Data().As<String>();
    std::string logType(*String::Utf8Value(logTypeLocal));
    cout << "[console." << logType << "] ";

    for (int i = 0; i < args.Length(); ++i) {
        Local<Value> v = args[i];
        if (v->IsString()) {
            String::Utf8Value str(v.As<String>());
            cout << *str;
        } else {
            String::Utf8Value str(v->ToString());
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

    exports->Set(String::NewFromUtf8(isolate, "info"), Function::New(context, logFunction).ToLocalChecked());
    exports->Set(String::NewFromUtf8(isolate, "log"), Function::New(context, logFunction).ToLocalChecked());
    exports->Set(String::NewFromUtf8(isolate, "warn"), Function::New(context, logFunction).ToLocalChecked());
    exports->Set(String::NewFromUtf8(isolate, "error"), Function::New(context, logFunction).ToLocalChecked());

    return handleScope.Escape(exports);
}
