#include "ast.hpp"

using namespace std;

AstNode::AstNode(AstNodeType type)
    : type{ type }
{
}

AstNodeType AstNode::getType() {
    return type;
}

AstRoot::AstRoot(vector<AstNode*> body)
    : AstNode(AstNodeType::Root)
    , body{ body }
{
}

const std::vector<AstNode*>& AstRoot::getBody() {
    return body;
}

Identifier::Identifier(string name)
    : AstNode(AstNodeType::Identifier)
    , name{ name }
{
}

RegExpLiteral::RegExpLiteral(string pattern, string flags)
    : AstNode(AstNodeType::RegExpLiteral)
    , pattern{ pattern }
    , flags{ flags }
{
}

NullLiteral::NullLiteral()
    : AstNode(AstNodeType::NullLiteral)
{
}

StringLiteral::StringLiteral(string value)
    : AstNode(AstNodeType::StringLiteral)
    , value{ value }
{
}

BooleanLiteral::BooleanLiteral(bool value)
    : AstNode(AstNodeType::BooleanLiteral)
    , value{ value }
{
}

NumericLiteral::NumericLiteral(double value)
    : AstNode(AstNodeType::NumericLiteral)
    , value{ value }
{
}

TemplateLiteral::TemplateLiteral(std::vector<AstNode*> quasis, std::vector<AstNode*> expressions)
    : AstNode(AstNodeType::TemplateLiteral)
    , quasis{ quasis }
    , expressions{ expressions }
{
}

TemplateElement::TemplateElement(std::string rawValue, bool isTail)
    : AstNode(AstNodeType::TemplateElement)
    , rawValue{ rawValue }
    , isTail{ isTail }
{
}

TaggedTemplateExpression::TaggedTemplateExpression(AstNode* tag, AstNode* quasi)
    : AstNode(AstNodeType::TaggedTemplateExpression)
    , tag{ tag }
    , quasi{ quasi }
{
}

ObjectProperty::ObjectProperty(AstNode* key, AstNode* value, bool isShorthand, bool isComputed)
    : AstNode(AstNodeType::ObjectProperty)
    , key{ key }
    , value{ value }
    , isShorthand{ isShorthand }
    , isComputed{ isComputed }
{
}

ObjectMethod::ObjectMethod(AstNode* id, vector<AstNode*> params, AstNode* body, AstNode* key, ObjectMethod::Kind kind,
    bool isGenerator, bool isAsync, bool isComputed)
    : Function(AstNodeType::ObjectMethod, id, params, body, isGenerator, isAsync)
    , key{ key }
    , kind{ kind }
    , isComputed{ isComputed }
{
}

ExpressionStatement::ExpressionStatement(AstNode* expression)
    : AstNode(AstNodeType::ExpressionStatement)
    , expression{ expression }
{
}

BlockStatement::BlockStatement(std::vector<AstNode*> body)
    : AstNode(AstNodeType::BlockStatement)
    , body{ body }
{
}

EmptyStatement::EmptyStatement()
    : AstNode(AstNodeType::EmptyStatement)
{
}

WithStatement::WithStatement(AstNode* object, AstNode* body)
    : AstNode(AstNodeType::WithStatement)
    , object{ object }
    , body{ body }
{
}

DebuggerStatement::DebuggerStatement()
    : AstNode(AstNodeType::DebuggerStatement)
{
}

ReturnStatement::ReturnStatement(AstNode* argument)
    : AstNode(AstNodeType::ReturnStatement)
    , argument{ argument }
{
}

LabeledStatement::LabeledStatement(AstNode* label, AstNode* body)
    : AstNode(AstNodeType::LabeledStatement)
    , label{ label }
    , body{ body }
{
}

BreakStatement::BreakStatement(AstNode* label)
    : AstNode(AstNodeType::BreakStatement)
    , label{ label }
{
}

ContinueStatement::ContinueStatement(AstNode* label)
    : AstNode(AstNodeType::ContinueStatement)
    , label{ label }
{
}

IfStatement::IfStatement(AstNode* test, AstNode* consequent, AstNode* argument)
    : AstNode(AstNodeType::IfStatement)
    , test{ test }
    , consequent{ consequent }
    , argument{ argument }
{
}

SwitchStatement::SwitchStatement(AstNode* discriminant, std::vector<AstNode*> cases)
    : AstNode(AstNodeType::SwitchStatement)
    , discriminant{ discriminant }
    , cases{ cases }
{
}

SwitchCase::SwitchCase(AstNode* testOrDefault, std::vector<AstNode*> consequent)
    : AstNode(AstNodeType::SwitchCase)
    , testOrDefault{ testOrDefault }
    , consequent{ consequent }
{
}

ThrowStatement::ThrowStatement(AstNode* argument)
    : AstNode(AstNodeType::ThrowStatement)
    , argument{ argument }
{
}

TryStatement::TryStatement(AstNode* body, AstNode* handler, AstNode* finalizer)
    : AstNode(AstNodeType::TryStatement)
    , body{ body }
    , handler{ handler }
    , finalizer{ finalizer }
{
}

CatchClause::CatchClause(AstNode* param, AstNode* body)
    : AstNode(AstNodeType::CatchClause)
    , param{ param }
    , body{ body }
{
}

WhileStatement::WhileStatement(AstNode* test, AstNode* body)
    : AstNode(AstNodeType::WhileStatement)
    , test{ test }
    , body{ body }
{
}

DoWhileStatement::DoWhileStatement(AstNode* test, AstNode* body)
    : AstNode(AstNodeType::DoWhileStatement)
    , test{ test }
    , body{ body }
{
}

ForStatement::ForStatement(AstNode* init, AstNode* test, AstNode* update, AstNode* body)
    : AstNode(AstNodeType::ForStatement)
    , init{ init }
    , test{ init }
    , update{ update }
    , body{ body }
{
}

ForInStatement::ForInStatement(AstNode* left, AstNode* right, AstNode* body)
    : AstNode(AstNodeType::ForInStatement)
    , left{ left }
    , right{ right }
    , body{ body }
{
}

ForOfStatement::ForOfStatement(AstNode* left, AstNode* right, AstNode* body, bool isAwait)
    : AstNode(AstNodeType::ForOfStatement)
    , left{ left }
    , right{ right }
    , body{ body }
    , isAwait{ isAwait }
{
}

Function::Function(AstNodeType type, AstNode* id, std::vector<AstNode*> params, AstNode* body, bool isGenerator, bool isAsync)
    : AstNode(type)
    , id{ id }
    , params{ params }
    , body{ body }
    , isGenerator{ isGenerator }
    , isAsync{ isAsync }
{
}

Super::Super()
    : AstNode(AstNodeType::Super)
{
}

Import::Import()
    : AstNode(AstNodeType::Import)
{
}

ThisExpression::ThisExpression()
    : AstNode(AstNodeType::ThisExpression)
{
}

ArrowFunctionExpression::ArrowFunctionExpression(AstNode* id, vector<AstNode*> params, AstNode* body, bool isGenerator, bool isAsync, bool isExpression)
    : Function(AstNodeType::ArrowFunctionExpression, id, params, body, isGenerator, isAsync)
    , isExpression{ isExpression }
{
}

YieldExpression::YieldExpression(AstNode* argument, bool isDelegate)
    : AstNode(AstNodeType::YieldExpression)
    , argument{ argument }
    , isDelegate{ isDelegate }
{
}

AwaitExpression::AwaitExpression(AstNode* argument)
    : AstNode(AstNodeType::AwaitExpression)
    , argument{ argument }
{
}

ArrayExpression::ArrayExpression(vector<AstNode*> elements)
    : AstNode(AstNodeType::ArrayExpression)
    , elements{ elements }
{
}

ObjectExpression::ObjectExpression(vector<AstNode*> properties)
    : AstNode(AstNodeType::ObjectExpression)
    , properties{ properties }
{
}

FunctionExpression::FunctionExpression(AstNode* id, vector<AstNode*> params, AstNode* body, bool isGenerator, bool isAsync)
    : Function(AstNodeType::FunctionExpression, id, params, body, isGenerator, isAsync)
{
}

UnaryExpression::UnaryExpression(AstNode* argument, Operator unaryOperator, bool isPrefix)
    : AstNode(AstNodeType::UnaryExpression)
    , argument{ argument }
    , unaryOperator{ unaryOperator }
    , isPrefix{ isPrefix }
{
}

UpdateExpression::UpdateExpression(AstNode* argument, Operator updateOperator, bool isPrefix)
    : AstNode(AstNodeType::UpdateExpression)
    , argument{ argument }
    , updateOperator{ updateOperator }
    , isPrefix{ isPrefix }
{
}

BinaryExpression::BinaryExpression(AstNode* left, AstNode* right, Operator binaryOperator)
    : AstNode(AstNodeType::BinaryExpression)
    , left{ left }
    , right{ right }
    , binaryOperator{ binaryOperator }
{
}

AssignmentExpression::AssignmentExpression(AstNode* left, AstNode* right, Operator assignmentOperator)
    : AstNode(AstNodeType::AssignmentExpression)
    , left{ left }
    , right{ right }
    , assignmentOperator{ assignmentOperator }
{
}

LogicalExpression::LogicalExpression(AstNode* left, AstNode* right, Operator logicalOperator)
    : AstNode(AstNodeType::LogicalExpression)
    , left{ left }
    , right{ right }
    , logicalOperator{ logicalOperator }
{
}

MemberExpression::MemberExpression(AstNode* object, AstNode* property, bool isComputed)
    : AstNode(AstNodeType::MemberExpression)
    , object{ object }
    , property{ property }
    , isComputed{ isComputed }
{
}

BindExpression::BindExpression(AstNode* object, AstNode* callee)
    : AstNode(AstNodeType::BindExpression)
    , object{ object }
    , callee{ callee }
{
}

ConditionalExpression::ConditionalExpression(AstNode* test, AstNode* alternate, AstNode* consequent)
    : AstNode(AstNodeType::ConditionalExpression)
    , test{ test }
    , alternate{ alternate }
    , consequent{ consequent }
{
}

CallExpression::CallExpression(AstNode* callee, vector<AstNode*> arguments)
    : CallExpression(AstNodeType::CallExpression, callee, arguments)
{
}

CallExpression::CallExpression(AstNodeType type, AstNode* callee, vector<AstNode*> arguments)
    : AstNode(type)
    , callee{ callee }
    , arguments{ arguments }
{
}

NewExpression::NewExpression(AstNode* callee, vector<AstNode*> arguments)
    : CallExpression(AstNodeType::NewExpression, callee, arguments)
{
}

SequenceExpression::SequenceExpression(std::vector<AstNode*> expressions)
    : AstNode(AstNodeType::SequenceExpression)
    , expressions{ expressions }
{
}

DoExpression::DoExpression(AstNode* body)
    : AstNode(AstNodeType::DoExpression)
    , body{ body }
{
}

Class::Class(AstNodeType type, AstNode* id, AstNode* superClass, AstNode* body)
    : AstNode(type)
    , id{ id }
    , superClass{ superClass }
    , body{ body }
{
}

ClassExpression::ClassExpression(AstNode* id, AstNode* superClass, AstNode* body)
    : Class(AstNodeType::ClassExpression, id, superClass, body)
{
}

ClassBody::ClassBody(std::vector<AstNode*> body)
    : AstNode(AstNodeType::ClassBody)
    , body{ body }
{
}

ClassMethod::ClassMethod(AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key, ClassMethod::Kind kind,
    bool isGenerator, bool isAsync, bool isComputed, bool isStatic)
    : Function(AstNodeType::ClassMethod, id, params, body, isGenerator, isAsync)
    , key{ key }
    , kind{ kind }
    , isComputed{ isComputed }
    , isStatic{ isStatic }
{
}

ClassPrivateMethod::ClassPrivateMethod(AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key, ClassPrivateMethod::Kind kind,
    bool isGenerator, bool isAsync, bool isStatic)
    : Function(AstNodeType::ClassPrivateMethod, id, params, body, isGenerator, isAsync)
    , key{ key }
    , kind{ kind }
    , isStatic{ isStatic }
{
}

ClassProperty::ClassProperty(AstNode* key, AstNode* value, bool isStatic, bool isComputed)
    : AstNode(AstNodeType::ClassProperty)
    , key{ key }
    , value{ value }
    , isStatic{ isStatic }
    , isComputed{ isComputed }
{
}

ClassPrivateProperty::ClassPrivateProperty(AstNode* key, AstNode* value, bool isStatic)
    : AstNode(AstNodeType::ClassPrivateProperty)
    , key{ key }
    , value{ value }
    , isStatic{ isStatic }
{
}

ClassDeclaration::ClassDeclaration(AstNode* id, AstNode* superClass, AstNode* body)
    : Class(AstNodeType::ClassDeclaration, id, superClass, body)
{
}

FunctionDeclaration::FunctionDeclaration(AstNode* id, vector<AstNode*> params, AstNode* body, bool isGenerator, bool isAsync)
    : Function(AstNodeType::FunctionDeclaration, id, params, body, isGenerator, isAsync)
{
}

VariableDeclaration::VariableDeclaration(vector<AstNode*> declarators, Kind kind)
    : AstNode(AstNodeType::VariableDeclaration)
    , declarators{ declarators }
    , kind{ kind }
{
}

VariableDeclarator::VariableDeclarator(AstNode* id, AstNode* init)
    : AstNode(AstNodeType::VariableDeclarator)
    , id{ id }
    , init{ init }
{
}

SpreadElement::SpreadElement(AstNode* argument)
    : AstNode(AstNodeType::SpreadElement)
    , argument{ argument }
{
}

ObjectPattern::ObjectPattern(std::vector<AstNode*> properties)
    : AstNode{ AstNodeType::ObjectPattern }
    , properties{ properties }
{
}

ArrayPattern::ArrayPattern(std::vector<AstNode*> elements)
    : AstNode{ AstNodeType::ArrayPattern }
    , elements{ elements }
{
}

AssignmentPattern::AssignmentPattern(AstNode* left, AstNode* right)
    : AstNode(AstNodeType::AssignmentPattern)
    , left{ left }
    , right{ right }
{
}

RestElement::RestElement(AstNode* argument)
    : AstNode(AstNodeType::RestElement)
    , argument{ argument }
{
}

MetaProperty::MetaProperty(AstNode* meta, AstNode* property)
    : AstNode(AstNodeType::MetaProperty)
    , meta{ meta }
    , property{ property }
{
}

ImportDeclaration::ImportDeclaration(std::vector<AstNode*> specifiers, AstNode* source)
    : AstNode(AstNodeType::ImportDeclaration)
    , specifiers{ specifiers }
    , source{ source }
{
}

ImportSpecifier::ImportSpecifier(AstNode* local, AstNode* imported)
    : AstNode(AstNodeType::ImportSpecifier)
    , local{ local }
    , imported{ imported }
{
}

ImportDefaultSpecifier::ImportDefaultSpecifier(AstNode* local)
    : AstNode(AstNodeType::ImportDefaultSpecifier)
    , local{ local }
{
}

ImportNamespaceSpecifier::ImportNamespaceSpecifier(AstNode* local)
    : AstNode(AstNodeType::ImportNamespaceSpecifier)
    , local{ local }
{
}

ExportNamedDeclaration::ExportNamedDeclaration(AstNode* declaration, AstNode* source, std::vector<AstNode*> specifiers)
    : AstNode(AstNodeType::ExportNamedDeclaration)
    , declaration{ declaration }
    , source{ source }
    , specifiers{ specifiers }
{
}

ExportDefaultDeclaration::ExportDefaultDeclaration(AstNode* declaration)
    : AstNode(AstNodeType::ExportDefaultDeclaration)
    , declaration{ declaration }
{
}

ExportAllDeclaration::ExportAllDeclaration(AstNode* source)
    : AstNode(AstNodeType::ExportAllDeclaration)
    , source{ source }
{
}

ExportSpecifier::ExportSpecifier(AstNode* local, AstNode* exported)
    : AstNode(AstNodeType::ExportSpecifier)
    , local{ local }
    , exported{ exported }
{
}
