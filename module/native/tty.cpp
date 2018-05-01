#include "isolatewrapper.hpp"
#include "module/native/modules.hpp"
#include <v8.h>

using namespace v8;

extern "C" {
extern const char ttyModuleScript[];
}

Local<Object> ttyModuleTemplate(IsolateWrapper& isolateWrapper)
{
    Isolate* isolate = *isolateWrapper;
    EscapableHandleScope handleScope(isolate);
    Local<Object> compiledScript = compileNativeModuleScript(isolateWrapper, ttyModuleScript);

    Local<Object> exports = Object::New(isolate);

    exportNativeModuleCompiledFunction(exports, compiledScript, "isatty");

    return handleScope.Escape(exports);
}
