#ifndef BASICMODULE_HPP
#define BASICMODULE_HPP

#include <experimental/filesystem>
#include <string>
#include <unordered_map>
#include <v8.h>

class IsolateWrapper;

class BasicModule {
public:
    BasicModule(IsolateWrapper& isolateWrapper);
    virtual ~BasicModule() = default;
    virtual std::string getPath() const = 0;
    IsolateWrapper& getIsolateWrapper() const;
    const v8::Local<v8::Object> getExports();

private:
    virtual void resolveExports() = 0;

protected:
    IsolateWrapper& isolateWrapper;
    v8::Isolate* isolate;
    v8::Persistent<v8::Context> persistentContext;

private:
    bool exportsResolveStarted;
};

#endif // BASICMODULE_HPP
