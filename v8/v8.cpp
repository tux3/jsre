#include "v8.hpp"
#include <libplatform/libplatform.h>
#include <string>

extern "C" {
extern const char v8_startup_natives_blob[];
extern const char v8_startup_natives_blob_end;
extern const char v8_startup_snapshot_blob[];
extern const char v8_startup_snapshot_blob_end;
}

V8::V8()
    : platform(v8::platform::NewDefaultPlatform())
{
    std::string flags = "--harmony_dynamic_import --harmony_class_fields";
    v8::V8::SetFlagsFromString(flags.c_str(), flags.length());
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    int v8_startup_snapshot_blob_size = static_cast<int>(&v8_startup_snapshot_blob_end - v8_startup_snapshot_blob);
    v8::StartupData snapshotBlobStartupData{v8_startup_snapshot_blob, v8_startup_snapshot_blob_size};
    v8::V8::SetSnapshotDataBlob(&snapshotBlobStartupData);

    int v8_startup_natives_blob_size = static_cast<int>(&v8_startup_natives_blob_end - v8_startup_natives_blob);
    v8::StartupData nativesBlobStartupData{v8_startup_natives_blob, v8_startup_natives_blob_size};
    v8::V8::SetNativesDataBlob(&nativesBlobStartupData);

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
