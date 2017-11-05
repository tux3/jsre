#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <v8.h>

class IsolateWrapper;

v8::Local<v8::Context> prepareGlobalContext(IsolateWrapper& isolate);

#endif // GLOBAL_HPP
