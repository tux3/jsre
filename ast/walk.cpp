#include "ast/walk.hpp"
#include "ast/ast.hpp"
#include "utils/utils.hpp"

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

template <class T>
static bool applyNode(const std::function<bool (AstNode *)> &cb, T* node)
{
    if (!node)
        return true;
    return cb(node);
}

template <class T>
static bool applyArray(const std::function<bool (AstNode *)> &cb, const std::vector<T*>& nodes)
{
    for (AstNode* child : nodes) {
        if (!child)
            continue;
        if (!cb(child))
            return false;
    }
    return true;
}

std::vector<AstNode*> AstNode::getChildren()
{
    vector<AstNode*> result;
    applyChildren([&](AstNode* child) {
        result.push_back(child);
        return true;
    });
    return result;
}

void AstNode::applyChildren(const std::function<bool (AstNode *)> &)
{
}

void AstRoot::applyChildren(const std::function<bool (AstNode*)>& cb) {
    applyArray(cb, body);
}

void Identifier::applyChildren(const std::function<bool (AstNode*)>& cb) {
    applyNode(cb, typeAnnotation);
}

void TemplateLiteral::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, quasis))
        return;
    if (!applyArray(cb, expressions))
        return;
}

void TaggedTemplateExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, tag))
        return;
    if (!applyNode(cb, quasi))
        return;
}

void Function::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, id))
        return;
    if (!applyArray(cb, params))
        return;
    if (!applyNode(cb, body))
        return;
    if (!applyNode(cb, typeParameters))
        return;
    if (!applyNode(cb, returnType))
        return;
}

void ObjectProperty::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, key))
        return;
    if (!applyNode(cb, value))
        return;
}

void ObjectMethod::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, key))
        return;
    Function::applyChildren(cb);
}

void ExpressionStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, expression))
        return;
}

void BlockStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, body))
        return;
}

void WithStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, object))
        return;
    if (!applyNode(cb, body))
        return;
}

void ReturnStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, argument))
        return;
}

void LabeledStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, label))
        return;
    if (!applyNode(cb, body))
        return;
}

void BreakStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    applyNode(cb, label);
}

void ContinueStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    applyNode(cb, label);
}

void IfStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, test))
        return;
    if (!applyNode(cb, consequent))
        return;
    if (!applyNode(cb, alternate))
        return;
}

void SwitchStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, discriminant))
        return;
    if (!applyArray(cb, cases))
        return;
}

void SwitchCase::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, testOrDefault))
        return;
    if (!applyArray(cb, consequent))
        return;
}

void ThrowStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, argument))
        return;
}

void TryStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, block))
        return;
    if (!applyNode(cb, handler))
        return;
    if (!applyNode(cb, finalizer))
        return;
}

void CatchClause::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, param))
        return;
    if (!applyNode(cb, body))
        return;
}

void WhileStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, test))
        return;
    if (!applyNode(cb, body))
        return;
}

void DoWhileStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, test))
        return;
    if (!applyNode(cb, body))
        return;
}

void ForStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, init))
        return;
    if (!applyNode(cb, test))
        return;
    if (!applyNode(cb, update))
        return;
    if (!applyNode(cb, body))
        return;
}

void ForInStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, left))
        return;
    if (!applyNode(cb, right))
        return;
    if (!applyNode(cb, body))
        return;
}

void ForOfStatement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, left))
        return;
    if (!applyNode(cb, right))
        return;
    if (!applyNode(cb, body))
        return;
}

void YieldExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, argument))
        return;
}

void AwaitExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, argument))
        return;
}

void ArrayExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, elements))
        return;
}

void ObjectExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, properties))
        return;
}

void UnaryExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, argument))
        return;
}

void UpdateExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, argument))
        return;
}

void BinaryExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, left))
        return;
    if (!applyNode(cb, right))
        return;
}

void AssignmentExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, left))
        return;
    if (!applyNode(cb, right))
        return;
}

void LogicalExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, left))
        return;
    if (!applyNode(cb, right))
        return;
}

void MemberExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, object))
        return;
    if (!applyNode(cb, property))
        return;
}

void BindExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, object))
        return;
    if (!applyNode(cb, callee))
        return;
}

void ConditionalExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, test))
        return;
    if (!applyNode(cb, alternate))
        return;
    if (!applyNode(cb, consequent))
        return;
}

void CallExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, callee))
        return;
    if (!applyArray(cb, arguments))
        return;
}

void SequenceExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, expressions))
        return;
}

void DoExpression::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, body))
        return;
}

void Class::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, implements))
        return;
    if (!applyNode(cb, id))
        return;
    if (!applyNode(cb, superClass))
        return;
    if (!applyNode(cb, body))
        return;
    if (!applyNode(cb, typeParameters))
        return;
    if (!applyNode(cb, superTypeParameters))
        return;
}

void ClassBody::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, body))
        return;
}

void ClassProperty::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, key))
        return;
    if (!applyNode(cb, value))
        return;
    if (!applyNode(cb, typeAnnotation))
        return;
}

void ClassPrivateProperty::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, key))
        return;
    if (!applyNode(cb, value))
        return;
    if (!applyNode(cb, typeAnnotation))
        return;
}

void ClassMethod::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, key))
        return;
    if (!applyNode(cb, returnType))
        return;
    Function::applyChildren(cb);
}

void ClassPrivateMethod::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, key))
        return;
    if (!applyNode(cb, returnType))
        return;
    Function::applyChildren(cb);
}

void VariableDeclaration::applyChildren(const std::function<bool (AstNode*)>& cb) {
    applyArray(cb, declarators);
}

void VariableDeclarator::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, id))
        return;
    if (!applyNode(cb, init))
        return;
}

void SpreadElement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, argument))
        return;
}

void ObjectPattern::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, properties))
        return;
    if (!applyNode(cb, typeAnnotation))
        return;
}

void ArrayPattern::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, elements))
        return;
}

void AssignmentPattern::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, left))
        return;
    if (!applyNode(cb, right))
        return;
}

void RestElement::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, argument))
        return;
    if (!applyNode(cb, typeAnnotation))
        return;
}

void MetaProperty::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, meta))
        return;
    if (!applyNode(cb, property))
        return;
}

void ImportDeclaration::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, specifiers))
        return;
    if (!applyNode(cb, source))
        return;
}

void ImportSpecifier::applyChildren(const std::function<bool (AstNode*)>& cb) {
    // We don't want to walk through two identifiers when there's only one written down in the source code
    // Having the imported available on demand is nice for consistency, but not when walking the AST
    if (!applyNode(cb, local))
        return;
    if (localEqualsImported)
        return;
    applyNode(cb, imported);
}

void ImportDefaultSpecifier::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, local))
        return;
}

void ImportNamespaceSpecifier::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, local))
        return;
}

void ExportNamedDeclaration::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, declaration))
        return;
    if (!applyNode(cb, source))
        return;
    if (!applyArray(cb, specifiers))
        return;
}

void ExportDefaultDeclaration::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, declaration))
        return;
}

void ExportAllDeclaration::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, source))
        return;
}

void ExportSpecifier::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, local))
        return;
    if (!applyNode(cb, exported))
        return;
}

void ExportDefaultSpecifier::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, exported))
        return;
}

void TypeAnnotation::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, typeAnnotation))
        return;
}

void GenericTypeAnnotation::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, id))
        return;
    if (!applyNode(cb, typeParameters))
        return;
}

void FunctionTypeAnnotation::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, params))
        return;
    if (!applyNode(cb, rest))
        return;
    if (!applyNode(cb, returnType))
        return;
}

void FunctionTypeParam::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, name))
        return;
    if (!applyNode(cb, typeAnnotation))
        return;
}

void ObjectTypeAnnotation::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyArray(cb, properties))
        return;
    if (!applyArray(cb, indexers))
        return;
}

void ObjectTypeProperty::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, key))
        return;
    if (!applyNode(cb, value))
        return;
}

void ObjectTypeSpreadProperty::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, argument))
        return;
}

void ObjectTypeIndexer::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, id))
        return;
    if (!applyNode(cb, key))
        return;
    if (!applyNode(cb, value))
        return;
}

void TypeAlias::applyChildren(const std::function<bool (AstNode*)>& cb) {
    if (!applyNode(cb, id))
        return;
    if (!applyNode(cb, typeParameters))
        return;
    if (!applyNode(cb, right))
        return;
}

void TypeParameterInstantiation::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyArray(cb, params))
        return;
}

void TypeParameterDeclaration::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyArray(cb, params))
        return;
}

void TypeCastExpression::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, expression))
        return;
    if (!applyNode(cb, typeAnnotation))
        return;
}

void NullableTypeAnnotation::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, typeAnnotation))
        return;
}

void ArrayTypeAnnotation::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, elementType))
        return;
}

void TupleTypeAnnotation::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyArray(cb, types))
        return;
}

void UnionTypeAnnotation::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyArray(cb, types))
        return;
}

void ClassImplements::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, id))
        return;
    if (!applyNode(cb, typeParameters))
        return;
}

void QualifiedTypeIdentifier::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, qualification))
        return;
    if (!applyNode(cb, id))
        return;
}

void TypeofTypeAnnotation::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, argument))
        return;
}

void InterfaceDeclaration::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, id))
        return;
    if (!applyNode(cb, typeParameters))
        return;
    if (!applyNode(cb, body))
        return;
    if (!applyArray(cb, extends))
        return;
    if (!applyArray(cb, mixins))
        return;
}

void InterfaceExtends::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, id))
        return;
    if (!applyNode(cb, typeParameters))
        return;
}

void TypeParameter::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, name))
        return;
    if (!applyNode(cb, bound))
        return;
}

void DeclareVariable::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, id))
        return;
}

void DeclareFunction::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, id))
        return;
}

void DeclareTypeAlias::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, id))
        return;
    if (!applyNode(cb, right))
        return;
}

void DeclareClass::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, id))
        return;
    if (!applyNode(cb, typeParameters))
        return;
    if (!applyNode(cb, body))
        return;
    if (!applyArray(cb, extends))
        return;
    if (!applyArray(cb, mixins))
        return;
}

void DeclareModule::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, id))
        return;
    if (!applyNode(cb, body))
        return;
}

void DeclareExportDeclaration::applyChildren(const std::function<bool (AstNode*)>& cb)
{
    if (!applyNode(cb, declaration))
        return;
}

