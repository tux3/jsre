#include "isolatewrapper.hpp"
#include "module/native/modules.hpp"
#include <v8.h>

using namespace v8;

extern "C" {
extern const char utilModuleScript[];
}

Local<Object> utilModuleTemplate(IsolateWrapper& isolateWrapper)
{
    Isolate* isolate = *isolateWrapper;
    EscapableHandleScope handleScope(isolate);
    Local<Object> compiledScript = compileNativeModuleScript(isolateWrapper, utilModuleScript);

    Local<Object> exports = Object::New(isolate);

    exportNativeModuleCompiledFunction(exports, compiledScript, "inherits");
    exportNativeModuleCompiledFunction(exports, compiledScript, "_extend");

    return handleScope.Escape(exports);
}
