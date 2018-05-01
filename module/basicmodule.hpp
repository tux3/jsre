#ifndef BASICMODULE_HPP
#define BASICMODULE_HPP

#include <string>
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
    virtual void evaluate() = 0;

protected:
    IsolateWrapper& isolateWrapper;
    v8::Isolate* isolate;
    v8::Persistent<v8::Context> persistentContext;

private:
    bool exportsResolveStarted;
};

#endif // BASICMODULE_HPP
