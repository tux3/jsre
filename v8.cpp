#include "v8.hpp"
#include <libplatform/libplatform.h>
#include <string>

V8::V8()
    : platform(v8::platform::CreateDefaultPlatform())
{
    std::string flags = "--harmony_dynamic_import --harmony_class_fields";
    v8::V8::SetFlagsFromString(flags.c_str(), flags.length());
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
}

V8::~V8()
{
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
}

const V8& V8::getInstance()
{
    static V8 singleton;
    return singleton;
}

const v8::Isolate::CreateParams& V8::getCreateParams() const
{
    return create_params;
}
