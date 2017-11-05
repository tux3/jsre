#include "isolatewrapper.hpp"
#include "module/native/modules.hpp"
#include <array>
#include <v8.h>

using namespace v8;

Local<Object> fsModuleTemplate(IsolateWrapper& isolateWrapper)
{
    Isolate* isolate = *isolateWrapper;
    EscapableHandleScope handleScope(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    Local<Object> exports = Object::New(isolate);

    std::array trapFunctions = {
        "read",
        "readSync",
        "readFile",
        "readFileSync",
        "ReadStream",
        "write",
        "writeSync",
        "writeFile",
        "writeFileSync",
        "WriteStream",
        "stat",
        "statSync",
        "readlink",
        "readlinkSync",
    };
    for (auto&& f : trapFunctions)
        exportNativeModuleTrapFunction(exports, f);

    return handleScope.Escape(exports);
}
