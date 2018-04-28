#include "ast/walk.hpp"
#include "utils.hpp"
#include <iostream>

using namespace std;
using json = nlohmann::json;

void walkAst(AstNode& node, AstNodeCallback cb, std::function<bool(AstNode&)> predicate)
{
    if (predicate(node))
        cb(node);
    for (auto child : node.getChildren())
        if (child)
            walkAst(*child, cb, predicate);
}

vector<AstNode*> AstNode::getChildren() {
    return {};
}

vector<AstNode*> AstRoot::getChildren() {
    return body;
}

vector<AstNode*> TemplateLiteral::getChildren() {
    return concat(quasis, expressions);
}

vector<AstNode*> TaggedTemplateExpression::getChildren() {
    return {tag, quasi};
}

vector<AstNode*> Function::getChildren() {
    auto children = concat({id}, params);
    children.push_back(body);
    return children;
}

vector<AstNode*> ObjectProperty::getChildren() {
    return {key, value};
}

vector<AstNode*> ObjectMethod::getChildren() {
    return concat({key}, Function::getChildren());
}

vector<AstNode*> ExpressionStatement::getChildren() {
    return {expression};
}

vector<AstNode*> BlockStatement::getChildren() {
    return {body};
}

vector<AstNode*> WithStatement::getChildren() {
    return {object, body};
}

vector<AstNode*> ReturnStatement::getChildren() {
    return {argument};
}

vector<AstNode*> LabeledStatement::getChildren() {
    return {label, body};
}

vector<AstNode*> BreakStatement::getChildren() {
    return {label};
}

vector<AstNode*> ContinueStatement::getChildren() {
    return {label};
}

vector<AstNode*> IfStatement::getChildren() {
    return {test, consequent, argument};
}

vector<AstNode*> SwitchStatement::getChildren() {
    return concat({discriminant}, cases);
}

vector<AstNode*> SwitchCase::getChildren() {
    return concat({testOrDefault}, consequent);
}

vector<AstNode*> ThrowStatement::getChildren() {
    return {argument};
}

vector<AstNode*> TryStatement::getChildren() {
    return {body, handler, finalizer};
}

vector<AstNode*> CatchClause::getChildren() {
    return {param, body};
}

vector<AstNode*> WhileStatement::getChildren() {
    return {test, body};
}

vector<AstNode*> DoWhileStatement::getChildren() {
    return {test, body};
}

vector<AstNode*> ForStatement::getChildren() {
    return {init, test, update, body};
}

vector<AstNode*> ForInStatement::getChildren() {
    return {left, right, body};
}

vector<AstNode*> ForOfStatement::getChildren() {
    return {left, right, body};
}

vector<AstNode*> YieldExpression::getChildren() {
    return {argument};
}

vector<AstNode*> AwaitExpression::getChildren() {
    return {argument};
}

vector<AstNode*> ArrayExpression::getChildren() {
    return elements;
}

vector<AstNode*> ObjectExpression::getChildren() {
    return properties;
}

vector<AstNode*> UnaryExpression::getChildren() {
    return {argument};
}

vector<AstNode*> UpdateExpression::getChildren() {
    return {argument};
}

vector<AstNode*> BinaryExpression::getChildren() {
    return {left, right};
}

vector<AstNode*> AssignmentExpression::getChildren() {
    return {left, right};
}

vector<AstNode*> LogicalExpression::getChildren() {
    return {left, right};
}

vector<AstNode*> MemberExpression::getChildren() {
    return {object, property};
}

vector<AstNode*> BindExpression::getChildren() {
    return {object, callee};
}

vector<AstNode*> ConditionalExpression::getChildren() {
    return {test, alternate, consequent};
}

vector<AstNode*> CallExpression::getChildren() {
    return concat({callee}, arguments);
}

vector<AstNode*> SequenceExpression::getChildren() {
    return expressions;
}

vector<AstNode*> DoExpression::getChildren() {
    return {body};
}

vector<AstNode*> Class::getChildren() {
    return {id, superClass, body};
}

vector<AstNode*> ClassBody::getChildren() {
    return body;
}

vector<AstNode*> ClassProperty::getChildren() {
    return {key, value};
}

vector<AstNode*> ClassPrivateProperty::getChildren() {
    return {key, value};
}

vector<AstNode*> ClassMethod::getChildren() {
    return concat({key}, Function::getChildren());
}

vector<AstNode*> ClassPrivateMethod::getChildren() {
    return concat({key}, Function::getChildren());
}

vector<AstNode*> VariableDeclaration::getChildren() {
    return declarators;
}

vector<AstNode*> VariableDeclarator::getChildren() {
    return {id, init};
}

vector<AstNode*> SpreadElement::getChildren() {
    return {argument};
}

vector<AstNode*> ObjectPattern::getChildren() {
    return properties;
}

vector<AstNode*> ArrayPattern::getChildren() {
    return elements;
}

vector<AstNode*> AssignmentPattern::getChildren() {
    return {left, right};
}

vector<AstNode*> RestElement::getChildren() {
    return {argument};
}

vector<AstNode*> MetaProperty::getChildren() {
    return {meta, property};
}

vector<AstNode*> ImportDeclaration::getChildren() {
    return concat(specifiers, {source});
}

vector<AstNode*> ImportSpecifier::getChildren() {
    // We don't want to walk through two identifiers when there's only one written down in the source code
    // Having the imported available on demand is nice for consistency, but not when walking the AST
    if (localEqualsImported)
        return {local};
    else
        return {local, imported};
}

vector<AstNode*> ImportDefaultSpecifier::getChildren() {
    return {local};
}

vector<AstNode*> ImportNamespaceSpecifier::getChildren() {
    return {local};
}

vector<AstNode*> ExportNamedDeclaration::getChildren() {
    return concat({declaration, source}, specifiers);
}

vector<AstNode*> ExportDefaultDeclaration::getChildren() {
    return {declaration};
}

vector<AstNode*> ExportAllDeclaration::getChildren() {
    return {source};
}

vector<AstNode*> ExportSpecifier::getChildren() {
    return {local, exported};
}

vector<AstNode*> ExportDefaultSpecifier::getChildren() {
    return {exported};
}
