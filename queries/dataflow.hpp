#ifndef DATAFLOW_HPP
#define DATAFLOW_HPP

#include "maybe.hpp"

class AstNode;

// True if the node is an expression or identifier unconditionally returned from the current scope
Tribool isReturnedValue(AstNode& node);

#endif // DATAFLOW_HPP
