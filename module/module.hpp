#ifndef MODULE_HPP
#define MODULE_HPP

#include "basicmodule.hpp"
#include <experimental/filesystem>
#include <json.hpp>
#include <string>
#include <v8.h>

class IsolateWrapper;
class AstRoot;

class Module final : public BasicModule {
public:
    Module(IsolateWrapper& isolateWrapper, std::experimental::filesystem::path path);
    const AstRoot& getAst();
    v8::Local<v8::Module> getCompiledModule();
    v8::Local<v8::Module> getCompiledES6Module();
    virtual std::string getPath() const override;
    void reconstructTypes();

    enum class EmbedderDataIndex : int {
        Reserved = 0, // Has a special meaning for the Chrome Debugger, or so I'm told
        ModulePath = 1,
    };

private:
    v8::Local<v8::Module> compileModuleFromSource(const std::string& filename, const std::string& source);
    std::string resolveImports(v8::Local<v8::Context>& context);
    virtual void resolveExports() override;
    bool isES6Module();
    void compileModule();

private:
    std::experimental::filesystem::path path;
    std::string originalSource, transpiledSource;
    nlohmann::json jast;
    AstRoot* ast;
    v8::Persistent<v8::Module> compiledModule;
    v8::Persistent<v8::Module> compiledThunkModule; //< ES6 thunk generated if this module doesn't use ES6 import/exports
};

#endif // MODULE_HPP
