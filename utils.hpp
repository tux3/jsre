#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace v8 {
class Isolate;
class TryCatch;
}

std::string readFileStr(const char* path);
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
