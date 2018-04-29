#include "global.hpp"
#include "isolatewrapper.hpp"
#include "module/nativemodule.hpp"
#include "module/moduleresolver.hpp"
#include <iostream>

using namespace std;
using namespace v8;

Local<Context> prepareGlobalContext(IsolateWrapper& isolateWrapper)
{
    Isolate* isolate = *isolateWrapper;
    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);

    static v8::Persistent<v8::Context> persistentContext;

    if (persistentContext.IsEmpty()) {
        Local<ObjectTemplate> globalTemplate = ObjectTemplate::New(isolate);
        Local<Context> context = Context::New(isolate, {}, globalTemplate);
        Context::Scope contextScope(context);
        Local<Object> global = context->Global();

        global->Set(String::NewFromUtf8(isolate, "global"), global);

        // Set up exports and module.exports
        Local<Object> exportsObj = Object::New(isolate);
        Local<Object> moduleObj = Object::New(isolate);
        moduleObj->Set(String::NewFromUtf8(isolate, "exports"), exportsObj);
        global->Set(String::NewFromUtf8(isolate, "exports"), exportsObj);
        global->Set(String::NewFromUtf8(isolate, "module"), moduleObj);

        // Load all native modules into global
        vector nativeModules = NativeModule::getNativeModuleNames();
        for (string name : nativeModules) {
            NativeModule mod(isolateWrapper, name);
            Local<Object> exports = mod.getExports();
            global->Set(String::NewFromUtf8(isolate, name.c_str()), exports);
        }

        // Load Buffer, the only global class that Node injects...
        // (yes, even though it's also available through the global buffer.Buffer)
        NativeModule bufferMod(isolateWrapper, "buffer");
        auto bufferClassStr = String::NewFromUtf8(isolate, "Buffer");
        global->Set(bufferClassStr, bufferMod.getExports()->Get(bufferClassStr));

        persistentContext.Reset(isolate, context);
    }

    return handleScope.Escape(persistentContext.Get(isolate));
}
