#include "astqueries.hpp"
#include "ast/ast.hpp"
#include <algorithm>

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

bool isUnscopedTypeIdentifier(Identifier& node)
{
    auto parent = node.getParent();
    if (parent->getType() == AstNodeType::FunctionTypeParam)
        return ((FunctionTypeParam*)parent)->getName() == &node;
    else if (parent->getType() == AstNodeType::ObjectTypeProperty)
        return ((ObjectTypeProperty*)parent)->getKey() == &node;
    else if (parent->getType() == AstNodeType::ObjectTypeIndexer)
        return ((ObjectTypeIndexer*)parent)->getId() == &node;
    else
        return false;
}

bool isFunctionalExpressionArgumentIdentifier(Identifier &node)
{
    auto parentType = node.getParent()->getType();
    return parentType == AstNodeType::ArrowFunctionExpression
            || parentType == AstNodeType::FunctionExpression;
}

bool isFunctionParameterIdentifier(Identifier &node)
{
    auto& parent = *node.getParent();
    if (!isFunctionNode(parent))
        return false;
    auto& fun = (Function&)parent;
    const auto& params = fun.getParams();
    return find(params.begin(), params.end(), &node) != params.end();
}

bool isFunctionNode(AstNode &node)
{
    auto type = node.getType();
    return type == AstNodeType::ArrowFunctionExpression
            || type == AstNodeType::FunctionExpression
            || type == AstNodeType::FunctionDeclaration
            || type == AstNodeType::ClassMethod
            || type == AstNodeType::ClassPrivateMethod
            || type == AstNodeType::ObjectMethod;
}
