#ifndef NATIVEMODULE_HPP
#define NATIVEMODULE_HPP

#include "basicmodule.hpp"
#include "module/native/modules.hpp"
#include <unordered_map>

class NativeModule final : public BasicModule {
public:
    NativeModule(IsolateWrapper& isolateWrapper, std::string name);
    virtual std::string getPath() const override;
    // The wrapper module is an ES6 module that just require()s the native module and re-exports it
    v8::MaybeLocal<v8::Module> getWrapperModule();

    static std::vector<std::string> getNativeModuleNames();
    static bool hasModule(const std::string &name);

private:
    virtual void resolveExports() override;
    std::string name;
    NativeModuleTemplate moduleTemplate;
};

#endif // NATIVEMODULE_HPP
