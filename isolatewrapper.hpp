#ifndef ISOLATEWRAPPER_HPP
#define ISOLATEWRAPPER_HPP

#include <json.hpp>
#include <v8.h>

class IsolateWrapper {
public:
    IsolateWrapper();
    ~IsolateWrapper();

    v8::Isolate* get();
    v8::Isolate* operator*() { return get(); }

private:
    v8::Persistent<v8::Context> defaultContext;
    v8::Isolate* isolate;
};

#endif // ISOLATEWRAPPER_HPP
