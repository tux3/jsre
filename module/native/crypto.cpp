#include "isolatewrapper.hpp"
#include "module/native/modules.hpp"
#include <v8.h>

using namespace std;
using namespace v8;

void randomBytes(const FunctionCallbackInfo<Value>& args)
{
    if (args.Length() != 1)
        throw runtime_error("crypto.randomBytes is only implemented for one argument!");

    Local<Value> sizeArg = args[0];
    if (!sizeArg->IsNumber())
        throw runtime_error("crypto.randomBytes called with a non-numeric size");
    double dsize = sizeArg->NumberValue(args.GetIsolate()->GetCurrentContext()).ToChecked();
    if (dsize < 0.0)
        throw runtime_error("crypto.randomBytes size must be positive");
    size_t size = static_cast<size_t>(dsize);

    // Not actually random! This is fine since scripts can't have side effects, they can't really use it for anything.
    Local<ArrayBuffer> buf = ArrayBuffer::New(args.GetIsolate(), size);
    Local<Uint8Array> array = Uint8Array::New(buf, 0, size);
    args.GetReturnValue().Set(array);
}

Local<Object> cryptoModuleTemplate(IsolateWrapper& isolateWrapper)
{
    Isolate* isolate = *isolateWrapper;
    EscapableHandleScope handleScope(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    Local<Object> exports = Object::New(isolate);

    exports->Set(String::NewFromUtf8(isolate, "randomBytes"), Function::New(context, randomBytes).ToLocalChecked());

    return handleScope.Escape(exports);
}
