#ifndef UTILS_H
#define UTILS_H

#include <string>

namespace v8 {
class Isolate;
class TryCatch;
}

std::string readFileStr(const char* path);
void reportV8Exception(v8::Isolate* isolate, v8::TryCatch* try_catch);

#endif // UTILS_H
