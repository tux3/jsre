#include "global.hpp"
#include "v8/isolatewrapper.hpp"
#include "module/nativemodule.hpp"
#include "module/moduleresolver.hpp"
#include <unordered_map>
#include <string>

using namespace std;
using namespace v8;

Local<Context> prepareGlobalContext(IsolateWrapper& isolateWrapper)
{
    Isolate* isolate = *isolateWrapper;
    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);

    static unordered_map<string, unique_ptr<v8::Persistent<v8::Object>>> persistentNativeExports;

    if (persistentNativeExports.empty()) {
        // Generate all native module exports once
        vector nativeModules = NativeModule::getNativeModuleNames();
        for (string name : nativeModules) {
            NativeModule mod(isolateWrapper, name);
            Local<Object> exports = mod.getExports();
            persistentNativeExports.insert({name, make_unique<Persistent<Object>>(isolate, exports)});
        }
    }

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

    // Add all native modules into global
    for (const auto& elem : persistentNativeExports)
        global->Set(String::NewFromUtf8(isolate, elem.first.c_str()), elem.second->Get(isolate));

    // Load Buffer, the only global class that Node injects...
    // (yes, even though it's also available through the global buffer.Buffer)
    auto bufferClassStr = String::NewFromUtf8(isolate, "Buffer");
    global->Set(bufferClassStr, global->Get(String::NewFromUtf8(isolate, "buffer")).As<Object>()->Get(bufferClassStr));

    return handleScope.Escape(context);
}
