#ifndef WALK_HPP
#define WALK_HPP

#include <functional>

class AstNode;

using AstNodeCallback = std::function<void(AstNode&)>;
void walkAst(AstNode& root, AstNodeCallback cb, std::function<bool(AstNode&)> predicate = [](auto&){return true;});

#endif // WALK_HPP
