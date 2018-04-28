#include "astqueries.hpp"
#include "ast/ast.hpp"

bool isUnscopedPropertyOrMethodIdentifier(Identifier& node)
{
    auto parent = node.getParent();
    if (parent->getType() == AstNodeType::ObjectProperty)
        return ((ObjectProperty*)parent)->getKey() == &node;
    else if (parent->getType() == AstNodeType::ClassProperty)
        return ((ClassProperty*)parent)->getKey() == &node;
    else if (parent->getType() == AstNodeType::ClassPrivateProperty)
        return ((ClassProperty*)parent)->getKey() == &node;
    else if (parent->getType() == AstNodeType::ClassMethod)
        return ((ClassMethod*)parent)->getKey() == &node;
    else if (parent->getType() == AstNodeType::ClassPrivateMethod)
        return ((ClassMethod*)parent)->getKey() == &node;
    else
        return false;
}

bool isFunctionalExpressionArgumentIdentifier(Identifier &node)
{
    auto parentType = node.getParent()->getType();
    return parentType == AstNodeType::ArrowFunctionExpression
            || parentType == AstNodeType::FunctionExpression;
}
