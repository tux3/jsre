#ifndef V8_HPP
#define V8_HPP

#include <memory>
#include <v8.h>

/// This singleton takes care of initializing/cleaning up V8
class V8
{
public:
    static const V8& getInstance();
    const v8::Isolate::CreateParams& getCreateParams() const;

private:
    V8();
    ~V8();

private:
    std::unique_ptr<v8::Platform> platform;
    v8::Isolate::CreateParams create_params;
};

#endif // V8_HPP
