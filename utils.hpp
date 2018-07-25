#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace v8 {
class Isolate;
class TryCatch;
}

std::optional<std::vector<uint8_t>> tryReadCacheFile(const char *name);
bool tryWriteCacheFile(const char* name, const std::vector<uint8_t>& data);
bool tryRemoveCacheFile(const char* name);
std::string readFileStr(const char* path);
void findSourceFiles(const std::filesystem::path& base, std::vector<std::string>& results);
void reportV8Exception(v8::Isolate* isolate, v8::TryCatch* try_catch);

template <class T>
std::vector<T> concat(std::vector<T> a, std::vector<T> b)
{
    std::vector<T> sum;
    sum.reserve(a.size() + b.size());
    sum.insert(sum.begin(), a.begin(), a.end());
    sum.insert(sum.begin(), b.begin(), b.end());
    return sum;
}

#endif // UTILS_H
