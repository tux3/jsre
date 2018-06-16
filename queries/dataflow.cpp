#include "dataflow.hpp"
#include "ast/ast.hpp"

Tribool isReturnedValue(AstNode &node)
{
    AstNode* parent = node.getParent();
    if (!parent)
        return Tribool::Nope;
    if (parent->getType() == AstNodeType::ReturnStatement
            || parent->getType() == AstNodeType::ArrowFunctionExpression)
        return Tribool::Yep;

    return Tribool::Maybe;
}
