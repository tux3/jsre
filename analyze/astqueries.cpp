#include "astqueries.hpp"
#include "ast/ast.hpp"
#include <algorithm>

bool isExternalIdentifier(Identifier& node)
{
    auto parent = node.getParent();
    if (parent->getType() == AstNodeType::ImportSpecifier) {
        return ((ImportSpecifier*)parent)->getImported() == &node;
    } else if (parent->getType() == AstNodeType::ImportDefaultSpecifier) {
        return ((ImportDefaultSpecifier*)parent)->getLocal() == &node;
    } else if (parent->getType() == AstNodeType::ExportDefaultSpecifier) {
        return ((ExportDefaultSpecifier*)parent)->getExported() == &node;
    } else if (parent->getType() == AstNodeType::ExportSpecifier) {
        auto exportNode = (ExportSpecifier*)parent;
        if (exportNode->getExported() == &node)
            return true;
        else if (exportNode->getLocal() == &node)
            return exportNode->getLocal()->getName() == exportNode->getExported()->getName();
        else
            return false;
    } else {
        return false;
    }
}

bool isUnscopedPropertyOrMethodIdentifier(Identifier& node)
{
    auto parent = node.getParent();
    auto parentType = parent->getType();
    if (parentType == AstNodeType::ObjectProperty)
        return ((ObjectProperty*)parent)->getKey() == &node;
    else if (parentType == AstNodeType::ClassProperty || parentType == AstNodeType::ClassPrivateProperty)
        return ((ClassBaseProperty*)parent)->getKey() == &node;
    else if (parentType == AstNodeType::ClassMethod || parentType == AstNodeType::ClassPrivateMethod)
        return ((ClassBaseMethod*)parent)->getKey() == &node;
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
    if (parentType != AstNodeType::ArrowFunctionExpression
            && parentType != AstNodeType::FunctionExpression)
        return false;
    auto& fun = (Function&)*node.getParent();
    const auto& params = fun.getParams();
    return find(params.begin(), params.end(), &node) != params.end();
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

bool isLexicalScopeNode(AstNode &node)
{
    if (isFunctionNode(node))
        return true;
    auto type = node.getType();
    return type == AstNodeType::ClassDeclaration
            || type == AstNodeType::ClassExpression;
}

bool isChildOf(AstNode* node, AstNode& reference)
{
    while (node) {
        if (node == &reference)
            return true;
        node = node->getParent();
    }
    return false;
}
