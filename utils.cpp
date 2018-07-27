#include "utils.hpp"
#include <fstream>
#include <filesystem>
#include <v8.h>

using namespace std;
namespace fs = std::filesystem;

static fs::path getCacheDirectory()
{
    static const std::string appName = "jsre";
    static auto fallback = fs::temp_directory_path();

#if defined(__linux__)
    char* home = getenv("HOME");
    if (!home)
        return fallback;
    return fs::path(home) / ".cache" / appName;
#elif defined(__APPLE__)
    char* home = getenv("HOME");
    if (!home)
        return fallback;
    return fs::path(home) / "Library" / "Caches" / appName;
#elif defined(_WIN32)
    char* home = getenv("USERPROFILE");
    if (!home)
        return fallback;
    return fs::path(home) / "AppData" / "Local" / appName;
#else
    auto
#endif
}

bool tryWriteCacheFile(const char *name, const std::vector<uint8_t> &data)
{
    auto dir = getCacheDirectory();
    fs::create_directories(dir);
    ofstream file(dir / name, ios::binary);
    if (!file.is_open())
        return false;
    file.write((const char*)data.data(), static_cast<ssize_t>(data.size()));
    return true;
}

bool tryRemoveCacheFile(const char *name)
{
    return fs::remove(getCacheDirectory() / name);
}

optional<vector<uint8_t>> tryReadCacheFile(const char *name)
{
    auto dir = getCacheDirectory();
    ifstream file(dir / name, ios::binary);
    if (!file.is_open())
        return nullopt;
    file.seekg(0, ios::end);
    auto size = file.tellg();
    vector<uint8_t> data(static_cast<size_t>(size));
    file.seekg(0, ios::beg);
    file.read((char*)data.data(), size);
    return data;
}

std::string readFileStr(const char* path)
{
    std::string str;
    ifstream file(path, ios::binary);
    if (!file.is_open())
        throw runtime_error("Couldn't open file");

    file.seekg(0, ios::end);
    auto size = file.tellg();
    str.resize(static_cast<size_t>(size));
    file.seekg(0, ios::beg);
    file.read(&str[0], size);
    return str;
}

void findSourceFiles(const fs::path& base, vector<fs::path>& results)
{
    if(!fs::exists(base) || !fs::is_directory(base))
        return;

    for (fs::recursive_directory_iterator it{base}, end; it != end; ++it) {
        if (fs::is_regular_file(*it)) {
            if (it->path().extension() == ".js")
                results.push_back(it->path());
        } else if (it->path().filename() == "node_modules") {
                it.disable_recursion_pending();
        }
    }
}

void reportV8Exception(v8::Isolate* isolate, v8::TryCatch* try_catch)
{
    v8::HandleScope handle_scope(isolate);
    v8::String::Utf8Value exception(isolate, try_catch->Exception());
    const char* exception_string = *exception;
    v8::Local<v8::Message> message = try_catch->Message();
    if (message.IsEmpty()) {
        // V8 didn't provide any extra information about this error; just
        // print the exception.
        fprintf(stderr, "%s\n", exception_string);
    } else {
        // Print (filename):(line number): (message).
        v8::String::Utf8Value filename(isolate, message->GetScriptOrigin().ResourceName());
        v8::Local<v8::Context> context(isolate->GetCurrentContext());
        const char* filename_string = *filename;
        int linenum = message->GetLineNumber(context).FromJust();
        fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
        // Print line of source code.
        v8::String::Utf8Value sourceline(isolate, message->GetSourceLine(context).ToLocalChecked());
        const char* sourceline_string = *sourceline;
        fprintf(stderr, "%s\n", sourceline_string);
        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn(context).FromJust();
        for (int i = 0; i < start; i++) {
            fprintf(stderr, " ");
        }
        int end = message->GetEndColumn(context).FromJust();
        for (int i = start; i < end; i++) {
            fprintf(stderr, "^");
        }
        fprintf(stderr, "\n");
        v8::Local<v8::Value> stack_trace_string;
        if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) && stack_trace_string->IsString() && v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
            v8::String::Utf8Value stack_trace(isolate, stack_trace_string);
            const char* stack_trace_string = *stack_trace;
            fprintf(stderr, "%s\n", stack_trace_string);
        }
    }
}
