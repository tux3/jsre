#ifndef WALK_HPP
#define WALK_HPP

#include <json.hpp>
#include <vector>
#include "ast/ast.hpp"

using AstNodeCallback = std::function<void(AstNode&)>;
void walkAst(AstNode& root, AstNodeCallback cb, std::function<bool(AstNode&)> predicate = [](auto&){return true;});

#endif // WALK_HPP
