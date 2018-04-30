#ifndef BABEL_H
#define BABEL_H

#include "isolatewrapper.hpp"
#include <filesystem>
#include <json.hpp>
#include <utility>
#include <future>

class AstRoot;
class Module;

void startParsingThreads();
void stopParsingThreads();

// This runs babel and imports its result in a worker thread, returning the AST
std::future<AstRoot*> parseSourceScriptAsync(Module& parentModule, const std::string& script);

v8::Local<v8::Object> makeBabelObject(IsolateWrapper& isolateWrapper);
nlohmann::json parseSourceScript(IsolateWrapper& isolateWrapper, v8::Local<v8::Object> babelObject, const std::string& scriptSource);

#endif // BABEL_H
