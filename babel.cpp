#include "babel.hpp"
#include "utils.hpp"
#include <iostream>
#include <v8.h>

using namespace v8;
using json = nlohmann::json;

extern "C" {
extern const char babelScriptStart[];
extern uint32_t babelScriptSize;
}

Local<Function> getBabelParseFunction(IsolateWrapper& isolateWrapper)
{
    Isolate* isolate = isolateWrapper.get();
    Isolate::Scope isolateScope(isolate);
    HandleScope handleScope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    TryCatch trycatch(isolate);

    static Persistent<Object> persistentBabelObj;
    if (persistentBabelObj.IsEmpty()) {
        Local<String> babelSourceStr = String::NewFromUtf8(isolate, babelScriptStart,
                                           NewStringType::kNormal, static_cast<int>(babelScriptSize))
                                           .ToLocalChecked();

        Local<Script> script;
        if (!Script::Compile(context, babelSourceStr).ToLocal(&script)) {
            reportV8Exception(isolate, &trycatch);
            throw std::runtime_error("transpileScript: Error compiling babel script");
        }

        Local<Object> babelObject = script->Run(context).ToLocalChecked().As<Object>();
        persistentBabelObj.Reset(isolate, babelObject);
    }

    Local<Object> babelObj = persistentBabelObj.Get(isolate);
    Local<String> transformFunctionName = String::NewFromUtf8(isolate, "transform");
    return babelObj->Get(context, transformFunctionName).ToLocalChecked().As<Function>();
}

std::pair<std::string, nlohmann::json> transpileScript(IsolateWrapper& isolateWrapper, const std::string& scriptSource)
{
    Isolate* isolate = isolateWrapper.get();
    Isolate::Scope isolateScope(isolate);
    HandleScope handleScope(isolate);
    Local<Context> context = Context::New(isolate);
    Context::Scope contextScope(context);
    TryCatch trycatch(isolate);

    Local<Value> scriptSourceStr = String::NewFromUtf8(isolate, scriptSource.data(),
                                       NewStringType::kNormal, scriptSource.size())
                                       .ToLocalChecked();
    Local<Value> transformOptions;
    if (!JSON::Parse(context, String::NewFromUtf8(isolate, R"({
        "sourceMaps": "false",
        "plugins": ["transform-flow-strip-types"],
        "parserOpts": {
            "plugins" : [
                "objectRestSpread",
                "classProperties",
                "exportExtensions",
                "asyncGenerators",
                "flow"
            ]
        }
    })")).ToLocal(&transformOptions)) {
        reportV8Exception(isolate, &trycatch);
        throw std::runtime_error("transpileScript: Failed to parse JSON options");
    }
    std::array arguments = { scriptSourceStr, transformOptions };

    Local<Function> parseFunction = getBabelParseFunction(isolateWrapper);

    Local<Value> result;
    if (!parseFunction.As<Function>()->Call(context, context->Global(), arguments.size(), arguments.data()).ToLocal(&result)
        || !result->IsObject()) {
        reportV8Exception(isolate, &trycatch);
        throw std::runtime_error("transpileScript: Failed to parse script");
    }

    Local<String> jsonStr = JSON::Stringify(context, result.As<Object>()).ToLocalChecked();
    String::Utf8Value jsonStrUtf8(jsonStr);
    json json = json::parse(*jsonStrUtf8, *jsonStrUtf8 + jsonStrUtf8.length());
    return { json["code"].get<std::string>(), json["ast"] };
}
