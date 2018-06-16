#include "ast/walk.hpp"
#include "ast/ast.hpp"
#include "utils.hpp"

using namespace std;

void walkAst(AstNode& node, AstNodeCallback cb, std::function<WalkDecision(AstNode&)> predicate)
{
    auto decision = predicate(node);
    if (decision == WalkDecision::WalkInto || decision == WalkDecision::WalkOver)
        cb(node);
    if (decision == WalkDecision::WalkOver || decision == WalkDecision::SkipOver)
        if (node.getType() != AstNodeType::Root) // Easy to forget, but we never want to skip the root's children
            return;
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

vector<AstNode*> Identifier::getChildren() {
    return {typeAnnotation};
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
    children.push_back(typeParameters);
    children.push_back(returnType);
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
    return concat((vector<AstNode*>&)implements, {id, superClass, body, typeParameters, superTypeParameters});
}

vector<AstNode*> ClassBody::getChildren() {
    return body;
}

vector<AstNode*> ClassProperty::getChildren() {
    return {key, value, typeAnnotation};
}

vector<AstNode*> ClassPrivateProperty::getChildren() {
    return {key, value, typeAnnotation};
}

vector<AstNode*> ClassMethod::getChildren() {
    return concat({key, returnType}, Function::getChildren());
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
    return concat(properties, {typeAnnotation});
}

vector<AstNode*> ArrayPattern::getChildren() {
    return elements;
}

vector<AstNode*> AssignmentPattern::getChildren() {
    return {left, right};
}

vector<AstNode*> RestElement::getChildren() {
    return {argument, typeAnnotation};
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

vector<AstNode*> TypeAnnotation::getChildren() {
    return {typeAnnotation};
}

vector<AstNode*> GenericTypeAnnotation::getChildren() {
    return {id, typeParameters};
}

vector<AstNode*> FunctionTypeAnnotation::getChildren() {
    return concat((vector<AstNode*>&)params, {(AstNode*)rest, returnType});
}

vector<AstNode*> FunctionTypeParam::getChildren() {
    return {name, typeAnnotation};
}

vector<AstNode*> ObjectTypeAnnotation::getChildren() {
    return concat((vector<AstNode*>&)properties, (vector<AstNode*>&)indexers);
}

vector<AstNode*> ObjectTypeProperty::getChildren() {
    return {key, value};
}

vector<AstNode*> ObjectTypeSpreadProperty::getChildren() {
    return {argument};
}

vector<AstNode*> ObjectTypeIndexer::getChildren() {
    return {id, key, value};
}

vector<AstNode*> TypeAlias::getChildren() {
    return {id, typeParameters, right};
}

vector<AstNode*> TypeParameterInstantiation::getChildren()
{
    return params;
}

vector<AstNode*> TypeParameterDeclaration::getChildren()
{
    return params;
}

vector<AstNode*> TypeCastExpression::getChildren()
{
    return {expression, typeAnnotation};
}

vector<AstNode*> NullableTypeAnnotation::getChildren()
{
    return {typeAnnotation};
}

vector<AstNode*> ArrayTypeAnnotation::getChildren()
{
    return {elementType};
}

vector<AstNode*> TupleTypeAnnotation::getChildren()
{
    return types;
}

vector<AstNode*> UnionTypeAnnotation::getChildren()
{
    return types;
}

vector<AstNode*> ClassImplements::getChildren()
{
    return {id, typeParameters};
}

vector<AstNode*> QualifiedTypeIdentifier::getChildren()
{
    return {qualification, id};
}

vector<AstNode*> TypeofTypeAnnotation::getChildren()
{
    return {argument};
}

vector<AstNode*> InterfaceDeclaration::getChildren()
{
    return concat(concat({id, (AstNode*)typeParameters, body},
                         (vector<AstNode*>&)extends),
                  (vector<AstNode*>&)mixins);
}

vector<AstNode*> InterfaceExtends::getChildren()
{
    return {id, typeParameters};
}

vector<AstNode*> TypeParameter::getChildren()
{
    return {name};
}

