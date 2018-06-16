#ifndef WALK_HPP
#define WALK_HPP

#include <functional>

class AstNode;

enum class WalkDecision: unsigned {
    WalkInto = 0b00, // Process this node and walk into children
    WalkOver = 0b01, // Process this node, skip children
    SkipInto = 0b10, // Skip this node but walk into children
    SkipOver = 0b11, // Skip this node and children
};

using AstNodeCallback = std::function<void(AstNode&)>;
void walkAst(AstNode& root, AstNodeCallback cb, std::function<WalkDecision(AstNode&)> predicate = [](auto&){return WalkDecision::WalkInto;});

#endif // WALK_HPP
