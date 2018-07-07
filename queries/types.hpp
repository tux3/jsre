#ifndef TYPES_HPP
#define TYPES_HPP

#include "maybe.hpp"

class Function;

// True if the function returns a Promise
Tribool returnsPromiseType(Function& fun);

#endif // TYPES_HPP
