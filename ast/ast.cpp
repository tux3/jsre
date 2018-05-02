#include "ast.hpp"
#include <cassert>

using namespace std;

AstNode::AstNode(AstSourceSpan location, AstNodeType type)
    : parent{ nullptr }
    , location{ location }
    , type{ type }
{
    assert(type < AstNodeType::Invalid);
}

AstNode* AstNode::getParent()
{
    return parent;
}

Module& AstNode::getParentModule()
{
    AstNode* node = this;
    while (node->type != AstNodeType::Root)
        node = node->getParent();
    auto root = reinterpret_cast<AstRoot*>(node);
    return root->getParentModule();
}

AstNodeType AstNode::getType() {
    return type;
}

const char* AstNode::getTypeName() {
    if (type == AstNodeType::Root)
        return "Root";
    #define X(IMPORTED_NODE_TYPE) \
        else if (type == AstNodeType::IMPORTED_NODE_TYPE) \
            return #IMPORTED_NODE_TYPE;
    IMPORTED_NODE_LIST(X)
    #undef X
    else
            throw new std::runtime_error("Unknown node type "+std::to_string((int)type));
}

AstSourceSpan AstNode::getLocation()
{
    return location;
}

void AstNode::setParentOfChildren() {
    for (auto child : getChildren())
        if (child)
            child->parent = this;
}

AstRoot::AstRoot(AstSourceSpan location, Module &parentModule, vector<AstNode*> body)
    : AstNode(location, AstNodeType::Root)
    , parentModule{ parentModule }
    , body{ move(body) }
{
    setParentOfChildren();
}

const std::vector<AstNode*>& AstRoot::getBody() {
    return body;
}

Module &AstRoot::getParentModule()
{
    return parentModule;
}

Identifier::Identifier(AstSourceSpan location, string name, TypeAnnotation* typeAnnotation, bool optional)
    : AstNode(location, AstNodeType::Identifier)
    , name{ move(name) }
    , typeAnnotation{ typeAnnotation }
    , optional{ optional }
{
    setParentOfChildren();
}

const std::string& Identifier::getName()
{
    return name;
}

TypeAnnotation *Identifier::getTypeAnnotation()
{
    return typeAnnotation;
}

bool Identifier::isOptional()
{
    return optional;
}

RegExpLiteral::RegExpLiteral(AstSourceSpan location, string pattern, string flags)
    : AstNode(location, AstNodeType::RegExpLiteral)
    , pattern{ move(pattern) }
    , flags{ flags }
{
    setParentOfChildren();
}

NullLiteral::NullLiteral(AstSourceSpan location)
    : AstNode(location, AstNodeType::NullLiteral)
{
    setParentOfChildren();
}

StringLiteral::StringLiteral(AstSourceSpan location, string value)
    : AstNode(location, AstNodeType::StringLiteral)
    , value{ move(value) }
{
    setParentOfChildren();
}

const string &StringLiteral::getValue()
{
    return value;
}

BooleanLiteral::BooleanLiteral(AstSourceSpan location, bool value)
    : AstNode(location, AstNodeType::BooleanLiteral)
    , value{ value }
{
    setParentOfChildren();
}

bool BooleanLiteral::getValue()
{
    return value;
}

NumericLiteral::NumericLiteral(AstSourceSpan location, double value)
    : AstNode(location, AstNodeType::NumericLiteral)
    , value{ value }
{
    setParentOfChildren();
}

double NumericLiteral::getValue()
{
    return value;
}

TemplateLiteral::TemplateLiteral(AstSourceSpan location, std::vector<AstNode*> quasis, std::vector<AstNode*> expressions)
    : AstNode(location, AstNodeType::TemplateLiteral)
    , quasis{ move(quasis) }
    , expressions{ move(expressions) }
{
    setParentOfChildren();
}

TemplateElement::TemplateElement(AstSourceSpan location, std::string rawValue, bool tail)
    : AstNode(location, AstNodeType::TemplateElement)
    , rawValue{ move(rawValue) }
    , tail{ tail }
{
    setParentOfChildren();
}

bool TemplateElement::isTail()
{
    return tail;
}

TaggedTemplateExpression::TaggedTemplateExpression(AstSourceSpan location, AstNode* tag, AstNode* quasi)
    : AstNode(location, AstNodeType::TaggedTemplateExpression)
    , tag{ tag }
    , quasi{ quasi }
{
    setParentOfChildren();
}

ObjectProperty::ObjectProperty(AstSourceSpan location, AstNode* key, AstNode* value, bool isShorthand, bool isComputed)
    : AstNode(location, AstNodeType::ObjectProperty)
    , key{ key }
    , value{ value }
    , isShorthand{ isShorthand }
    , isComputed{ isComputed }
{
    setParentOfChildren();
}

Identifier* ObjectProperty::getKey()
{
    return reinterpret_cast<Identifier*>(key);
}

ObjectMethod::ObjectMethod(AstSourceSpan location, AstNode* id, vector<AstNode*> params, AstNode* body, TypeParameterDeclaration* typeParameters,
                           TypeAnnotation* returnType, AstNode* key, ObjectMethod::Kind kind, bool isGenerator, bool isAsync, bool isComputed)
    : Function(location, AstNodeType::ObjectMethod, id, move(params), body, typeParameters, returnType, isGenerator, isAsync)
    , key{ key }
    , kind{ kind }
    , isComputed{ isComputed }
{
    setParentOfChildren();
}

ExpressionStatement::ExpressionStatement(AstSourceSpan location, AstNode* expression)
    : AstNode(location, AstNodeType::ExpressionStatement)
    , expression{ expression }
{
    setParentOfChildren();
}

BlockStatement::BlockStatement(AstSourceSpan location, std::vector<AstNode*> body)
    : AstNode(location, AstNodeType::BlockStatement)
    , body{ move(body) }
{
    setParentOfChildren();
}

EmptyStatement::EmptyStatement(AstSourceSpan location)
    : AstNode(location, AstNodeType::EmptyStatement)
{
    setParentOfChildren();
}

WithStatement::WithStatement(AstSourceSpan location, AstNode* object, AstNode* body)
    : AstNode(location, AstNodeType::WithStatement)
    , object{ object }
    , body{ body }
{
    setParentOfChildren();
}

DebuggerStatement::DebuggerStatement(AstSourceSpan location)
    : AstNode(location, AstNodeType::DebuggerStatement)
{
    setParentOfChildren();
}

ReturnStatement::ReturnStatement(AstSourceSpan location, AstNode* argument)
    : AstNode(location, AstNodeType::ReturnStatement)
    , argument{ argument }
{
    setParentOfChildren();
}

LabeledStatement::LabeledStatement(AstSourceSpan location, AstNode* label, AstNode* body)
    : AstNode(location, AstNodeType::LabeledStatement)
    , label{ label }
    , body{ body }
{
    setParentOfChildren();
}

BreakStatement::BreakStatement(AstSourceSpan location, AstNode* label)
    : AstNode(location, AstNodeType::BreakStatement)
    , label{ label }
{
    setParentOfChildren();
}

ContinueStatement::ContinueStatement(AstSourceSpan location, AstNode* label)
    : AstNode(location, AstNodeType::ContinueStatement)
    , label{ label }
{
    setParentOfChildren();
}

IfStatement::IfStatement(AstSourceSpan location, AstNode* test, AstNode* consequent, AstNode* argument)
    : AstNode(location, AstNodeType::IfStatement)
    , test{ test }
    , consequent{ consequent }
    , argument{ argument }
{
    setParentOfChildren();
}

SwitchStatement::SwitchStatement(AstSourceSpan location, AstNode* discriminant, std::vector<AstNode*> cases)
    : AstNode(location, AstNodeType::SwitchStatement)
    , discriminant{ discriminant }
    , cases{ move(cases) }
{
    setParentOfChildren();
}

SwitchCase::SwitchCase(AstSourceSpan location, AstNode* testOrDefault, std::vector<AstNode*> consequent)
    : AstNode(location, AstNodeType::SwitchCase)
    , testOrDefault{ testOrDefault }
    , consequent{ move(consequent) }
{
    setParentOfChildren();
}

const std::vector<AstNode *> &SwitchCase::getConsequent()
{
    return consequent;
}

ThrowStatement::ThrowStatement(AstSourceSpan location, AstNode* argument)
    : AstNode(location, AstNodeType::ThrowStatement)
    , argument{ argument }
{
    setParentOfChildren();
}

TryStatement::TryStatement(AstSourceSpan location, AstNode* body, AstNode* handler, AstNode* finalizer)
    : AstNode(location, AstNodeType::TryStatement)
    , body{ body }
    , handler{ handler }
    , finalizer{ finalizer }
{
    setParentOfChildren();
}

CatchClause::CatchClause(AstSourceSpan location, AstNode* param, AstNode* body)
    : AstNode(location, AstNodeType::CatchClause)
    , param{ param }
    , body{ body }
{
    setParentOfChildren();
}

AstNode* CatchClause::getParam()
{
    return param;
}

AstNode* CatchClause::getBody()
{
    return body;
}

WhileStatement::WhileStatement(AstSourceSpan location, AstNode* test, AstNode* body)
    : AstNode(location, AstNodeType::WhileStatement)
    , test{ test }
    , body{ body }
{
    setParentOfChildren();
}

DoWhileStatement::DoWhileStatement(AstSourceSpan location, AstNode* test, AstNode* body)
    : AstNode(location, AstNodeType::DoWhileStatement)
    , test{ test }
    , body{ body }
{
    setParentOfChildren();
}

ForStatement::ForStatement(AstSourceSpan location, AstNode* init, AstNode* test, AstNode* update, AstNode* body)
    : AstNode(location, AstNodeType::ForStatement)
    , init{ init }
    , test{ test }
    , update{ update }
    , body{ body }
{
    setParentOfChildren();
}

VariableDeclaration* ForStatement::getInit()
{
    return reinterpret_cast<VariableDeclaration*>(init);
}

ForInStatement::ForInStatement(AstSourceSpan location, AstNode* left, AstNode* right, AstNode* body)
    : AstNode(location, AstNodeType::ForInStatement)
    , left{ left }
    , right{ right }
    , body{ body }
{
    setParentOfChildren();
}

AstNode* ForInStatement::getLeft()
{
    return left;
}

ForOfStatement::ForOfStatement(AstSourceSpan location, AstNode* left, AstNode* right, AstNode* body, bool isAwait)
    : AstNode(location, AstNodeType::ForOfStatement)
    , left{ left }
    , right{ right }
    , body{ body }
    , isAwait{ isAwait }
{
    setParentOfChildren();
}

AstNode* ForOfStatement::getLeft()
{
    return left;
}

Function::Function(AstSourceSpan location, AstNodeType type, AstNode* id, std::vector<AstNode*> params, AstNode* body,
                   TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType, bool generator, bool async)
    : AstNode(location, type)
    , id{ id }
    , params{ move(params) }
    , body{ body }
    , typeParameters{ typeParameters }
    , returnType{ returnType }
    , generator{ generator }
    , async{ async }
{
}

bool Function::isGenerator()
{
    return generator;
}

bool Function::isAsync()
{
    return async;
}

Identifier* Function::getId()
{
    return reinterpret_cast<Identifier*>(id);
}

BlockStatement* Function::getBody()
{
    return reinterpret_cast<BlockStatement*>(body);
}

const std::vector<Identifier*>& Function::getParams()
{
    return reinterpret_cast<vector<Identifier*>&>(params);
}

Super::Super(AstSourceSpan location)
    : AstNode(location, AstNodeType::Super)
{
    setParentOfChildren();
}

Import::Import(AstSourceSpan location)
    : AstNode(location, AstNodeType::Import)
{
    setParentOfChildren();
}

ThisExpression::ThisExpression(AstSourceSpan location)
    : AstNode(location, AstNodeType::ThisExpression)
{
    setParentOfChildren();
}

ArrowFunctionExpression::ArrowFunctionExpression(AstSourceSpan location, AstNode* id, vector<AstNode*> params, AstNode* body, TypeParameterDeclaration* typeParameters,
                                                 TypeAnnotation* returnType, bool isGenerator, bool isAsync, bool expression)
    : Function(location, AstNodeType::ArrowFunctionExpression, id, move(params), body, typeParameters, returnType, isGenerator, isAsync)
    , expression{ expression }
{
    setParentOfChildren();
}

bool ArrowFunctionExpression::isExpression()
{
    return expression;
}

YieldExpression::YieldExpression(AstSourceSpan location, AstNode* argument, bool isDelegate)
    : AstNode(location, AstNodeType::YieldExpression)
    , argument{ argument }
    , isDelegate{ isDelegate }
{
    setParentOfChildren();
}

AwaitExpression::AwaitExpression(AstSourceSpan location, AstNode* argument)
    : AstNode(location, AstNodeType::AwaitExpression)
    , argument{ argument }
{
    setParentOfChildren();
}

ArrayExpression::ArrayExpression(AstSourceSpan location, vector<AstNode*> elements)
    : AstNode(location, AstNodeType::ArrayExpression)
    , elements{ move(elements) }
{
    setParentOfChildren();
}

ObjectExpression::ObjectExpression(AstSourceSpan location, vector<AstNode*> properties)
    : AstNode(location, AstNodeType::ObjectExpression)
    , properties{ move(properties) }
{
    setParentOfChildren();
}

FunctionExpression::FunctionExpression(AstSourceSpan location, AstNode* id, vector<AstNode*> params, AstNode* body,
                                       TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType, bool isGenerator, bool isAsync)
    : Function(location, AstNodeType::FunctionExpression, id, move(params), body, typeParameters, returnType, isGenerator, isAsync)
{
    setParentOfChildren();
}

UnaryExpression::UnaryExpression(AstSourceSpan location, AstNode* argument, Operator unaryOperator, bool isPrefix)
    : AstNode(location, AstNodeType::UnaryExpression)
    , argument{ argument }
    , unaryOperator{ unaryOperator }
    , isPrefix{ isPrefix }
{
    setParentOfChildren();
}

UpdateExpression::UpdateExpression(AstSourceSpan location, AstNode* argument, Operator updateOperator, bool isPrefix)
    : AstNode(location, AstNodeType::UpdateExpression)
    , argument{ argument }
    , updateOperator{ updateOperator }
    , isPrefix{ isPrefix }
{
    setParentOfChildren();
}

BinaryExpression::BinaryExpression(AstSourceSpan location, AstNode* left, AstNode* right, Operator binaryOperator)
    : AstNode(location, AstNodeType::BinaryExpression)
    , left{ left }
    , right{ right }
    , binaryOperator{ binaryOperator }
{
    setParentOfChildren();
}

AssignmentExpression::AssignmentExpression(AstSourceSpan location, AstNode* left, AstNode* right, Operator assignmentOperator)
    : AstNode(location, AstNodeType::AssignmentExpression)
    , left{ left }
    , right{ right }
    , assignmentOperator{ assignmentOperator }
{
    setParentOfChildren();
}

LogicalExpression::LogicalExpression(AstSourceSpan location, AstNode* left, AstNode* right, Operator logicalOperator)
    : AstNode(location, AstNodeType::LogicalExpression)
    , left{ left }
    , right{ right }
    , logicalOperator{ logicalOperator }
{
    setParentOfChildren();
}

MemberExpression::MemberExpression(AstSourceSpan location, AstNode* object, AstNode* property, bool isComputed)
    : AstNode(location, AstNodeType::MemberExpression)
    , object{ object }
    , property{ property }
    , isComputed{ isComputed }
{
    setParentOfChildren();
}

Identifier *MemberExpression::getProperty()
{
    return reinterpret_cast<Identifier*>(property);
}

BindExpression::BindExpression(AstSourceSpan location, AstNode* object, AstNode* callee)
    : AstNode(location, AstNodeType::BindExpression)
    , object{ object }
    , callee{ callee }
{
    setParentOfChildren();
}

ConditionalExpression::ConditionalExpression(AstSourceSpan location, AstNode* test, AstNode* alternate, AstNode* consequent)
    : AstNode(location, AstNodeType::ConditionalExpression)
    , test{ test }
    , alternate{ alternate }
    , consequent{ consequent }
{
    setParentOfChildren();
}

CallExpression::CallExpression(AstSourceSpan location, AstNode* callee, vector<AstNode*> arguments)
    : CallExpression(location, AstNodeType::CallExpression, callee, move(arguments))
{
    setParentOfChildren();
}

const std::vector<AstNode *> &CallExpression::getArguments()
{
    return arguments;
}

CallExpression::CallExpression(AstSourceSpan location, AstNodeType type, AstNode* callee, vector<AstNode*> arguments)
    : AstNode(location, type)
    , callee{ callee }
    , arguments{ move(arguments) }
{
}

NewExpression::NewExpression(AstSourceSpan location, AstNode* callee, vector<AstNode*> arguments)
    : CallExpression(location, AstNodeType::NewExpression, callee, move(arguments))
{
    setParentOfChildren();
}

SequenceExpression::SequenceExpression(AstSourceSpan location, std::vector<AstNode*> expressions)
    : AstNode(location, AstNodeType::SequenceExpression)
    , expressions{ move(expressions) }
{
    setParentOfChildren();
}

DoExpression::DoExpression(AstSourceSpan location, AstNode* body)
    : AstNode(location, AstNodeType::DoExpression)
    , body{ body }
{
    setParentOfChildren();
}

Class::Class(AstSourceSpan location, AstNodeType type, AstNode* id, AstNode* superClass, AstNode* body,
             TypeParameterDeclaration* typeParameters, TypeParameterInstantiation *superTypeParameters, std::vector<ClassImplements*> implements)
    : AstNode(location, type)
    , id{ id }
    , superClass{ superClass }
    , body{ body }
    , typeParameters{ typeParameters }
    , superTypeParameters{ superTypeParameters }
    , implements{ move(implements) }
{
}

Identifier* Class::getId()
{
    return reinterpret_cast<Identifier*>(id);
}

TypeParameterDeclaration *Class::getTypeParameters()
{
    return typeParameters;
}

const std::vector<ClassImplements*>& Class::getImplements()
{
    return implements;
}

ClassExpression::ClassExpression(AstSourceSpan location, AstNode* id, AstNode* superClass, AstNode* body,
                                 TypeParameterDeclaration *typeParameters, TypeParameterInstantiation *superTypeParameters, std::vector<ClassImplements*> implements)
    : Class(location, AstNodeType::ClassExpression, id, superClass, body, typeParameters, superTypeParameters, implements)
{
    setParentOfChildren();
}

ClassBody::ClassBody(AstSourceSpan location, std::vector<AstNode*> body)
    : AstNode(location, AstNodeType::ClassBody)
    , body{ move(body) }
{
    setParentOfChildren();
}

ClassMethod::ClassMethod(AstSourceSpan location, AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key,
                         TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType,
                         ClassMethod::Kind kind, bool isGenerator, bool isAsync, bool isComputed, bool isStatic)
    : Function(location, AstNodeType::ClassMethod, id, move(params), body, typeParameters, returnType, isGenerator, isAsync)
    , key{ key }
    , kind{ kind }
    , isComputed{ isComputed }
    , isStatic{ isStatic }
{
    setParentOfChildren();
}

Identifier *ClassMethod::getKey()
{
    return reinterpret_cast<Identifier*>(key);
}

ClassPrivateMethod::ClassPrivateMethod(AstSourceSpan location, AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key,
                                       TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType,
                                       ClassPrivateMethod::Kind kind, bool isGenerator, bool isAsync, bool isStatic)
    : Function(location, AstNodeType::ClassPrivateMethod, id, move(params), body, typeParameters, returnType, isGenerator, isAsync)
    , key{ key }
    , kind{ kind }
    , isStatic{ isStatic }
{
    setParentOfChildren();
}

Identifier* ClassPrivateMethod::getKey()
{
    return reinterpret_cast<Identifier*>(key);
}

ClassProperty::ClassProperty(AstSourceSpan location, AstNode* key, AstNode* value,
                             TypeAnnotation* typeAnnotation, bool isStatic, bool isComputed)
    : AstNode(location, AstNodeType::ClassProperty)
    , key{ key }
    , value{ value }
    , typeAnnotation{ typeAnnotation }
    , isStatic{ isStatic }
    , isComputed{ isComputed }
{
    setParentOfChildren();
}

Identifier* ClassProperty::getKey()
{
    return reinterpret_cast<Identifier*>(key);
}

ClassPrivateProperty::ClassPrivateProperty(AstSourceSpan location, AstNode* key, AstNode* value,
                                           TypeAnnotation* typeAnnotation, bool isStatic)
    : AstNode(location, AstNodeType::ClassPrivateProperty)
    , key{ key }
    , value{ value }
    , typeAnnotation{ typeAnnotation }
    , isStatic{ isStatic }
{
    setParentOfChildren();
}

Identifier* ClassPrivateProperty::getKey()
{
    return reinterpret_cast<Identifier*>(key);
}

ClassDeclaration::ClassDeclaration(AstSourceSpan location, AstNode* id, AstNode* superClass, AstNode* body,
                                   TypeParameterDeclaration *typeParameters, TypeParameterInstantiation *superTypeParameters, std::vector<ClassImplements*> implements)
    : Class(location, AstNodeType::ClassDeclaration, id, superClass, body, typeParameters, superTypeParameters, move(implements))
{
    setParentOfChildren();
}

FunctionDeclaration::FunctionDeclaration(AstSourceSpan location, AstNode* id, vector<AstNode*> params, AstNode* body,
                                         TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType, bool isGenerator, bool isAsync)
    : Function(location, AstNodeType::FunctionDeclaration, id, move(params), body, typeParameters, returnType, isGenerator, isAsync)
{
    setParentOfChildren();
}

VariableDeclaration::VariableDeclaration(AstSourceSpan location, vector<AstNode*> declarators, Kind kind)
    : AstNode(location, AstNodeType::VariableDeclaration)
    , declarators{ move(declarators) }
    , kind{ kind }
{
    setParentOfChildren();
}

VariableDeclaration::Kind VariableDeclaration::getKind()
{
    return kind;
}

const std::vector<AstNode*>& VariableDeclaration::getDeclarators()
{
    return declarators;
}

VariableDeclarator::VariableDeclarator(AstSourceSpan location, AstNode* id, AstNode* init)
    : AstNode(location, AstNodeType::VariableDeclarator)
    , id{ id }
    , init{ init }
{
    setParentOfChildren();
}

AstNode* VariableDeclarator::getId()
{
    return id;
}

SpreadElement::SpreadElement(AstSourceSpan location, AstNode* argument)
    : AstNode(location, AstNodeType::SpreadElement)
    , argument{ argument }
{
    setParentOfChildren();
}

ObjectPattern::ObjectPattern(AstSourceSpan location, std::vector<AstNode*> properties, TypeAnnotation* typeAnnotation)
    : AstNode(location, AstNodeType::ObjectPattern)
    , properties{ move(properties) }
    , typeAnnotation{ typeAnnotation }
{
    setParentOfChildren();
}

const std::vector<ObjectProperty *> &ObjectPattern::getProperties()
{
    return reinterpret_cast<vector<ObjectProperty*>&>(properties);
}

ArrayPattern::ArrayPattern(AstSourceSpan location, std::vector<AstNode*> elements)
    : AstNode(location, AstNodeType::ArrayPattern)
    , elements{ move(elements) }
{
    setParentOfChildren();
}

const std::vector<AstNode*>& ArrayPattern::getElements()
{
    return elements;
}

AssignmentPattern::AssignmentPattern(AstSourceSpan location, AstNode* left, AstNode* right)
    : AstNode(location, AstNodeType::AssignmentPattern)
    , left{ left }
    , right{ right }
{
    setParentOfChildren();
}

Identifier *AssignmentPattern::getLeft()
{
    return reinterpret_cast<Identifier*>(left);
}

AstNode *AssignmentPattern::getRight()
{
    return right;
}

RestElement::RestElement(AstSourceSpan location, AstNode* argument, TypeAnnotation *typeAnnotation)
    : AstNode(location, AstNodeType::RestElement)
    , argument{ argument }
    , typeAnnotation{ typeAnnotation }
{
    setParentOfChildren();
}

Identifier *RestElement::getArgument()
{
    return reinterpret_cast<Identifier*>(argument);
}

MetaProperty::MetaProperty(AstSourceSpan location, AstNode* meta, AstNode* property)
    : AstNode(location, AstNodeType::MetaProperty)
    , meta{ meta }
    , property{ property }
{
    setParentOfChildren();
}

ImportDeclaration::ImportDeclaration(AstSourceSpan location, std::vector<AstNode*> specifiers, AstNode* source, Kind kind)
    : AstNode(location, AstNodeType::ImportDeclaration)
    , specifiers{ move(specifiers) }
    , source{ source }
    , kind{ kind }
{
    setParentOfChildren();
}

string ImportDeclaration::getSource()
{
    assert(source->getType() == AstNodeType::StringLiteral);
    auto sourceLiteral = (StringLiteral*)source;
    return sourceLiteral->getValue();
}

ImportDeclaration::Kind ImportDeclaration::getKind()
{
    return kind;
}

const std::vector<AstNode*>& ImportDeclaration::getSpecifiers()
{
    return specifiers;
}

ImportSpecifier::ImportSpecifier(AstSourceSpan location, AstNode* local, AstNode* imported, bool typeImport)
    : AstNode(location, AstNodeType::ImportSpecifier)
    , local{ (Identifier*)local }
    , imported{ (Identifier*)imported }
    , localEqualsImported{ this->local->getName() == this->imported->getName() }
    , typeImport{ typeImport }
{
    assert(local->getType() == AstNodeType::Identifier);
    assert(imported->getType() == AstNodeType::Identifier);
    setParentOfChildren();
}

Identifier* ImportSpecifier::getLocal()
{
    return reinterpret_cast<Identifier*>(local);
}

Identifier *ImportSpecifier::getImported()
{
    return reinterpret_cast<Identifier*>(imported);
}

bool ImportSpecifier::isTypeImport()
{
    return typeImport;
}

ImportDefaultSpecifier::ImportDefaultSpecifier(AstSourceSpan location, AstNode* local)
    : AstNode(location, AstNodeType::ImportDefaultSpecifier)
    , local{ local }
{
    setParentOfChildren();
}

Identifier* ImportDefaultSpecifier::getLocal()
{
    return reinterpret_cast<Identifier*>(local);
}

ImportNamespaceSpecifier::ImportNamespaceSpecifier(AstSourceSpan location, AstNode* local)
    : AstNode(location, AstNodeType::ImportNamespaceSpecifier)
    , local{ local }
{
    setParentOfChildren();
}

Identifier* ImportNamespaceSpecifier::getLocal()
{
    return reinterpret_cast<Identifier*>(local);
}

ExportNamedDeclaration::ExportNamedDeclaration(AstSourceSpan location, AstNode* declaration, AstNode* source, std::vector<AstNode*> specifiers, Kind kind)
    : AstNode(location, AstNodeType::ExportNamedDeclaration)
    , declaration{ declaration }
    , source{ source }
    , specifiers{ move(specifiers) }
    , kind{ kind }
{
    setParentOfChildren();
}

ExportNamedDeclaration::Kind ExportNamedDeclaration::getKind()
{
    return kind;
}

ExportDefaultDeclaration::ExportDefaultDeclaration(AstSourceSpan location, AstNode* declaration)
    : AstNode(location, AstNodeType::ExportDefaultDeclaration)
    , declaration{ declaration }
{
    setParentOfChildren();
}

ExportAllDeclaration::ExportAllDeclaration(AstSourceSpan location, AstNode* source)
    : AstNode(location, AstNodeType::ExportAllDeclaration)
    , source{ source }
{
    setParentOfChildren();
}

ExportSpecifier::ExportSpecifier(AstSourceSpan location, AstNode* local, AstNode* exported)
    : AstNode(location, AstNodeType::ExportSpecifier)
    , local{ local }
    , exported{ exported }
{
    setParentOfChildren();
}

Identifier *ExportSpecifier::getLocal()
{
    return reinterpret_cast<Identifier*>(local);
}

Identifier *ExportSpecifier::getExported()
{
    return reinterpret_cast<Identifier*>(exported);
}

ExportDefaultSpecifier::ExportDefaultSpecifier(AstSourceSpan location, AstNode* exported)
    : AstNode(location, AstNodeType::ExportDefaultSpecifier)
    , exported{ exported }
{
    setParentOfChildren();
}

Identifier *ExportDefaultSpecifier::getExported()
{
    return reinterpret_cast<Identifier*>(exported);
}

TypeAnnotation::TypeAnnotation(AstSourceSpan location, AstNode *typeAnnotation)
    : AstNode(location, AstNodeType::TypeAnnotation)
    , typeAnnotation{ typeAnnotation }
{
    setParentOfChildren();
}

GenericTypeAnnotation::GenericTypeAnnotation(AstSourceSpan location, AstNode *id, AstNode *typeParameters)
    : AstNode(location, AstNodeType::GenericTypeAnnotation)
    , id{ id }
    , typeParameters{ typeParameters }
{
    setParentOfChildren();
}

StringTypeAnnotation::StringTypeAnnotation(AstSourceSpan location)
    : AstNode(location, AstNodeType::StringTypeAnnotation)
{
    setParentOfChildren();
}

NumberTypeAnnotation::NumberTypeAnnotation(AstSourceSpan location)
    : AstNode(location, AstNodeType::NumberTypeAnnotation)
{
    setParentOfChildren();
}

BooleanTypeAnnotation::BooleanTypeAnnotation(AstSourceSpan location)
    : AstNode(location, AstNodeType::BooleanTypeAnnotation)
{
    setParentOfChildren();
}

VoidTypeAnnotation::VoidTypeAnnotation(AstSourceSpan location)
    : AstNode(location, AstNodeType::VoidTypeAnnotation)
{
    setParentOfChildren();
}

AnyTypeAnnotation::AnyTypeAnnotation(AstSourceSpan location)
    : AstNode(location, AstNodeType::AnyTypeAnnotation)
{
    setParentOfChildren();
}

ExistsTypeAnnotation::ExistsTypeAnnotation(AstSourceSpan location)
    : AstNode(location, AstNodeType::ExistsTypeAnnotation)
{
    setParentOfChildren();
}

MixedTypeAnnotation::MixedTypeAnnotation(AstSourceSpan location)
    : AstNode(location, AstNodeType::MixedTypeAnnotation)
{
    setParentOfChildren();
}

NullableTypeAnnotation::NullableTypeAnnotation(AstSourceSpan location, AstNode* typeAnnotation)
    : AstNode(location, AstNodeType::NullableTypeAnnotation)
    , typeAnnotation{ typeAnnotation }
{
    setParentOfChildren();
}

TupleTypeAnnotation::TupleTypeAnnotation(AstSourceSpan location,std::vector<AstNode*> types)
    : AstNode(location, AstNodeType::TupleTypeAnnotation)
    , types{ move(types) }
{
    setParentOfChildren();
}

UnionTypeAnnotation::UnionTypeAnnotation(AstSourceSpan location,std::vector<AstNode*> types)
    : AstNode(location, AstNodeType::UnionTypeAnnotation)
    , types{ move(types) }
{
    setParentOfChildren();
}

NullLiteralTypeAnnotation::NullLiteralTypeAnnotation(AstSourceSpan location)
    : AstNode(location, AstNodeType::NullLiteralTypeAnnotation)
{
    setParentOfChildren();
}

NumberLiteralTypeAnnotation::NumberLiteralTypeAnnotation(AstSourceSpan location, double value)
    : AstNode(location, AstNodeType::NumberLiteralTypeAnnotation)
    , value{ value }
{
    setParentOfChildren();
}

double NumberLiteralTypeAnnotation::getValue()
{
    return value;
}

StringLiteralTypeAnnotation::StringLiteralTypeAnnotation(AstSourceSpan location, string value)
    : AstNode(location, AstNodeType::StringLiteralTypeAnnotation)
    , value{ move(value) }
{
    setParentOfChildren();
}

BooleanLiteralTypeAnnotation::BooleanLiteralTypeAnnotation(AstSourceSpan location, bool value)
    : AstNode(location, AstNodeType::BooleanLiteralTypeAnnotation)
    , value{ value }
{
    setParentOfChildren();
}

bool BooleanLiteralTypeAnnotation::getValue()
{
    return value;
}

TypeofTypeAnnotation::TypeofTypeAnnotation(AstSourceSpan location, AstNode* argument)
    : AstNode(location, AstNodeType::TypeofTypeAnnotation)
    , argument{ argument }
{
    setParentOfChildren();
}

FunctionTypeAnnotation::FunctionTypeAnnotation(AstSourceSpan location, std::vector<FunctionTypeParam *> params, FunctionTypeParam *rest, AstNode *returnType)
    : AstNode(location, AstNodeType::FunctionTypeAnnotation)
    , params{ move(params) }
    , rest{ rest }
    , returnType{ returnType }
{
    setParentOfChildren();
}

FunctionTypeParam::FunctionTypeParam(AstSourceSpan location, Identifier* name, AstNode* typeAnnotation)
    : AstNode(location, AstNodeType::FunctionTypeParam)
    , name{ name }
    , typeAnnotation{ typeAnnotation }
{
    setParentOfChildren();
}

Identifier *FunctionTypeParam::getName()
{
    return name;
}

ObjectTypeAnnotation::ObjectTypeAnnotation(AstSourceSpan location, std::vector<ObjectTypeProperty *> properties, std::vector<ObjectTypeIndexer *> indexers, bool exact)
    : AstNode(location, AstNodeType::ObjectTypeAnnotation)
    , properties{ move(properties) }
    , indexers{ move(indexers) }
    , exact{ exact }
{
    setParentOfChildren();
}

ObjectTypeProperty::ObjectTypeProperty(AstSourceSpan location, Identifier *key, AstNode *value, bool optional)
    : AstNode(location, AstNodeType::ObjectTypeProperty)
    , key{ key }
    , value{ value }
    , optional{ optional }
{
    setParentOfChildren();
}

Identifier *ObjectTypeProperty::getKey()
{
    return key;
}

AstNode *ObjectTypeProperty::getValue()
{
    return value;
}

bool ObjectTypeProperty::isOptional()
{
    return optional;
}

ObjectTypeIndexer::ObjectTypeIndexer(AstSourceSpan location, Identifier *id, AstNode *key, AstNode *value)
    : AstNode(location, AstNodeType::ObjectTypeIndexer)
    , id{ id }
    , key{ key }
    , value{ value }
{
    setParentOfChildren();
}

Identifier *ObjectTypeIndexer::getId()
{
    return id;
}

AstNode *ObjectTypeIndexer::getKey()
{
    return key;
}

AstNode *ObjectTypeIndexer::getValue()
{
    return value;
}

TypeAlias::TypeAlias(AstSourceSpan location, Identifier *id, AstNode *typeParameters, AstNode *right)
    : AstNode(location, AstNodeType::TypeAlias)
    , id{ id }
    , typeParameters{ typeParameters }
    , right{ right }
{
    setParentOfChildren();
}

Identifier *TypeAlias::getId()
{
    return id;
}

AstNode *TypeAlias::getTypeParameters()
{
    return typeParameters;
}

AstNode *TypeAlias::getRight()
{
    return right;
}

TypeParameterInstantiation::TypeParameterInstantiation(AstSourceSpan location, std::vector<AstNode *> params)
    : AstNode(location, AstNodeType::TypeParameterInstantiation)
    , params{ move(params) }
{
    setParentOfChildren();
}

TypeParameterDeclaration::TypeParameterDeclaration(AstSourceSpan location, std::vector<AstNode *> params)
    : AstNode(location, AstNodeType::TypeParameterDeclaration)
    , params{ move(params) }
{
    setParentOfChildren();
}

const std::vector<TypeParameter *> &TypeParameterDeclaration::getParams()
{
    return (const std::vector<TypeParameter *> &)params;
}

TypeParameter::TypeParameter(AstSourceSpan location, std::string name)
    : AstNode(location, AstNodeType::TypeParameter)
    , name{ new Identifier(location, move(name), nullptr, false) }
{
    setParentOfChildren();
}

Identifier *TypeParameter::getName()
{
    return name;
}

TypeCastExpression::TypeCastExpression(AstSourceSpan location, AstNode *expression, TypeAnnotation *typeAnnotation)
    : AstNode(location, AstNodeType::TypeCastExpression)
    , expression{ expression }
    , typeAnnotation{ typeAnnotation }
{
    setParentOfChildren();
}

ClassImplements::ClassImplements(AstSourceSpan location, Identifier *id, TypeParameterInstantiation* typeParameters)
    : AstNode(location, AstNodeType::ClassImplements)
    , id{ id }
    , typeParameters{ typeParameters }
{
    setParentOfChildren();
}

QualifiedTypeIdentifier::QualifiedTypeIdentifier(AstSourceSpan location, Identifier *qualification, Identifier *id)
    : AstNode(location, AstNodeType::QualifiedTypeIdentifier)
    , qualification{ qualification }
    , id{ id }
{
    setParentOfChildren();
}

Identifier *QualifiedTypeIdentifier::getId()
{
    return id;
}

Identifier *QualifiedTypeIdentifier::getQualification()
{
    return qualification;
}

InterfaceDeclaration::InterfaceDeclaration(AstSourceSpan location, Identifier *id, TypeParameterDeclaration *typeParameters,
                                           AstNode *body, std::vector<InterfaceExtends *> extends, std::vector<InterfaceExtends *> mixins)
    : AstNode(location, AstNodeType::InterfaceDeclaration)
    , id{ id }
    , typeParameters{ typeParameters }
    , body{ body }
    , extends{ move(extends) }
    , mixins{ move(mixins) }
{
    setParentOfChildren();
}

Identifier *InterfaceDeclaration::getId()
{
    return id;
}

InterfaceExtends::InterfaceExtends(AstSourceSpan location, Identifier *id, TypeParameterInstantiation* typeParameters)
    : AstNode(location, AstNodeType::InterfaceExtends)
    , id{ id }
    , typeParameters{ typeParameters }
{
    setParentOfChildren();
}
