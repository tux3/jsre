#include "isolatewrapper.hpp"
#include "module/native/modules.hpp"
#include <iostream>
#include <v8.h>

using namespace v8;

extern "C" {
extern const char bufferModuleScript[];
}

Local<Object> bufferModuleTemplate(IsolateWrapper& isolateWrapper)
{
    Isolate* isolate = *isolateWrapper;
    EscapableHandleScope handleScope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> compiledScript = compileNativeModuleScript(isolateWrapper, bufferModuleScript);

    Local<Object> exports = Object::New(isolate);

    exportNativeModuleCompiledFunction(exports, compiledScript, "Buffer");

    return handleScope.Escape(exports);
}
