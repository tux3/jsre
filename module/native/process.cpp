#include "v8/isolatewrapper.hpp"
#include "module/native/modules.hpp"
#include <v8.h>

using namespace v8;

static constexpr const char* EMULATED_NODE_VERSION = "8.9.0";

Local<Object> processModuleTemplate(IsolateWrapper& isolateWrapper)
{
    Isolate* isolate = *isolateWrapper;
    EscapableHandleScope handleScope(isolate);

    Local<Object> exports = Object::New(isolate);

    exports->Set(String::NewFromUtf8(isolate, "env"), Object::New(isolate));
    exports->Set(String::NewFromUtf8(isolate, "argv"), Array::New(isolate));
    exports->Set(String::NewFromUtf8(isolate, "version"), String::NewFromUtf8(isolate, EMULATED_NODE_VERSION));

    exportNativeModuleTrapFunction(exports, "exit");

    exportNativeModuleStubFunction(exports, "on");
    exportNativeModuleStubFunction(exports, "removeAllListeners");

    return handleScope.Escape(exports);
}
