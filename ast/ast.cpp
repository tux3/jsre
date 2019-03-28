#include "ast.hpp"
#include "module/module.hpp"
#include "utf8/utf8.h"
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

const AstNode *AstNode::getParent() const
{
    return parent;
}

Module& AstNode::getParentModule() const
{
    const AstNode* node = this;
    while (node->type != AstNodeType::Root)
        node = node->getParent();
    auto root = reinterpret_cast<const AstRoot*>(node);
    return root->getParentModule();
}

AstNodeType AstNode::getType() const {
    return type;
}

const char* AstNode::getTypeName() const {
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

AstSourceSpan AstNode::getLocation() const
{
    return location;
}

void AstNode::setParentOfChildren() {
    applyChildren([this](AstNode* child) {
        child->parent = this;
        return true;
    });
}

string AstNode::getSourceString()
{
    const string& source = getParentModule().getOriginalSource();

    const char* beg_ptr = source.data();
    utf8::unchecked::advance(beg_ptr, location.start.offset);
    const char* end_ptr = beg_ptr;
    utf8::unchecked::advance(end_ptr, location.end.offset - location.start.offset);

    return string(beg_ptr, end_ptr - beg_ptr);
}

AstComment::AstComment(AstSourceSpan location, AstComment::Type type, string text)
    : AstNode(location, type == Type::Line ? AstNodeType::CommentLine : AstNodeType::CommentBlock)
    , text{text},
      location{location},
      type{type}
{
}

string AstComment::getText() const
{
    return text;
}

AstComment::Type AstComment::getType() const
{
    return type;
}

AstRoot::AstRoot(AstSourceSpan location, Module &parentModule, vector<AstNode*> body, std::vector<AstComment*> comments)
    : AstNode(location, AstNodeType::Root)
    , parentModule{ parentModule }
    , body{ move(body) }
    , comments{ move(comments) }
{
    setParentOfChildren();
}

const std::vector<AstNode*>& AstRoot::getBody()
{
    return body;
}

const std::vector<AstComment*> &AstRoot::getComments()
{
    return comments;
}

Module &AstRoot::getParentModule() const
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

const string &RegExpLiteral::getPattern()
{
    return pattern;
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

const std::vector<AstNode *> &TemplateLiteral::getQuasis()
{
    return quasis;
}

const std::vector<AstNode *> &TemplateLiteral::getExpressions()
{
    return expressions;
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

ObjectProperty::ObjectProperty(AstSourceSpan location, AstNode* key, AstNode* value, bool shorthand, bool computed)
    : AstNode(location, AstNodeType::ObjectProperty)
    , key{ key }
    , value{ value }
    , shorthand{ shorthand }
    , computed{ computed }
{
    setParentOfChildren();
}

AstNode *ObjectProperty::getKey()
{
    return key;
}

Identifier *ObjectProperty::getValue()
{
    return reinterpret_cast<Identifier*>(value);
}

bool ObjectProperty::isShorthand()
{
    return shorthand;
}

bool ObjectProperty::isComputed()
{
    return computed;
}

ObjectMethod::ObjectMethod(AstSourceSpan location, AstNode* id, vector<AstNode*> params, AstNode* body, TypeParameterDeclaration* typeParameters,
                           TypeAnnotation* returnType, AstNode* key, ObjectMethod::Kind kind, bool isGenerator, bool isAsync, bool isComputed)
    : Function(location, AstNodeType::ObjectMethod, id, move(params), body, typeParameters, returnType, isGenerator, isAsync)
    , key{ key }
    , kind{ kind }
    , computed{ isComputed }
{
    setParentOfChildren();
}

AstNode *ObjectMethod::getKey()
{
    return key;
}

bool ObjectMethod::isComputed()
{
    return computed;
}

ExpressionStatement::ExpressionStatement(AstSourceSpan location, AstNode* expression)
    : AstNode(location, AstNodeType::ExpressionStatement)
    , expression{ expression }
{
    setParentOfChildren();
}

AstNode *ExpressionStatement::getExpression()
{
    return expression;
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

AstNode *ReturnStatement::getArgument()
{
    return argument;
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

AstNode *BreakStatement::getLabel()
{
    return label;
}

ContinueStatement::ContinueStatement(AstSourceSpan location, AstNode* label)
    : AstNode(location, AstNodeType::ContinueStatement)
    , label{ label }
{
    setParentOfChildren();
}

AstNode *ContinueStatement::getLabel()
{
    return label;
}

IfStatement::IfStatement(AstSourceSpan location, AstNode* test, AstNode* consequent, AstNode* alternate)
    : AstNode(location, AstNodeType::IfStatement)
    , test{ test }
    , consequent{ consequent }
    , alternate{ alternate }
{
    setParentOfChildren();
}

AstNode *IfStatement::getTest()
{
    return test;
}

AstNode *IfStatement::getConsequent()
{
    return consequent;
}

AstNode *IfStatement::getAlternate()
{
    return alternate;
}

SwitchStatement::SwitchStatement(AstSourceSpan location, AstNode* discriminant, std::vector<SwitchCase*> cases)
    : AstNode(location, AstNodeType::SwitchStatement)
    , discriminant{ discriminant }
    , cases{ move(cases) }
{
    setParentOfChildren();
}

AstNode *SwitchStatement::getDiscriminant()
{
    return discriminant;
}

const std::vector<SwitchCase*> &SwitchStatement::getCases()
{
    return cases;
}

SwitchCase::SwitchCase(AstSourceSpan location, AstNode* testOrDefault, std::vector<AstNode*> consequent)
    : AstNode(location, AstNodeType::SwitchCase)
    , testOrDefault{ testOrDefault }
    , consequent{ move(consequent) }
{
    setParentOfChildren();
}

AstNode *SwitchCase::getTest()
{
    return testOrDefault;
}

const std::vector<AstNode *> &SwitchCase::getConsequents()
{
    return consequent;
}

ThrowStatement::ThrowStatement(AstSourceSpan location, AstNode* argument)
    : AstNode(location, AstNodeType::ThrowStatement)
    , argument{ argument }
{
    setParentOfChildren();
}

AstNode *ThrowStatement::getArgument()
{
    return argument;
}

TryStatement::TryStatement(AstSourceSpan location, AstNode* block, AstNode* handler, AstNode* finalizer)
    : AstNode(location, AstNodeType::TryStatement)
    , block{ block }
    , handler{ handler }
    , finalizer{ finalizer }
{
    setParentOfChildren();
}

AstNode *TryStatement::getBlock()
{
    return block;
}

CatchClause *TryStatement::getHandler()
{
    return reinterpret_cast<CatchClause*>(handler);
}

AstNode *TryStatement::getFinalizer()
{
    return finalizer;
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

AstNode *WhileStatement::getTest()
{
    return test;
}

AstNode *WhileStatement::getBody()
{
    return body;
}

DoWhileStatement::DoWhileStatement(AstSourceSpan location, AstNode* test, AstNode* body)
    : AstNode(location, AstNodeType::DoWhileStatement)
    , test{ test }
    , body{ body }
{
    setParentOfChildren();
}

AstNode *DoWhileStatement::getTest()
{
    return test;
}

AstNode *DoWhileStatement::getBody()
{
    return body;
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

AstNode* ForStatement::getInit()
{
    return init;
}

AstNode *ForStatement::getTest()
{
    return test;
}

AstNode *ForStatement::getUpdate()
{
    return update;
}

AstNode *ForStatement::getBody()
{
    return body;
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

AstNode *ForInStatement::getRight()
{
    return right;
}

AstNode *ForInStatement::getBody()
{
    return body;
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

AstNode *ForOfStatement::getRight()
{
    return right;
}

AstNode *ForOfStatement::getBody()
{
    return body;
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

TypeAnnotation *Function::getReturnType()
{
    return returnType;
}

AstNode *Function::getReturnTypeAnnotation()
{
    if (TypeAnnotation* returnType = getReturnType())
        return returnType->getTypeAnnotation();
    return nullptr;
}

TypeParameterDeclaration *Function::getTypeParameters()
{
    return typeParameters;
}

Identifier* Function::getId()
{
    return reinterpret_cast<Identifier*>(id);
}

AstNode *Function::getBody()
{
    return body;
}

const std::vector<AstNode*>& Function::getParams()
{
    return params;
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

AstNode *AwaitExpression::getArgument()
{
    return argument;
}

ArrayExpression::ArrayExpression(AstSourceSpan location, vector<AstNode*> elements)
    : AstNode(location, AstNodeType::ArrayExpression)
    , elements{ move(elements) }
{
    setParentOfChildren();
}

const std::vector<AstNode *> &ArrayExpression::getElements()
{
    return elements;
}

ObjectExpression::ObjectExpression(AstSourceSpan location, vector<AstNode*> properties)
    : AstNode(location, AstNodeType::ObjectExpression)
    , properties{ move(properties) }
{
    setParentOfChildren();
}

const std::vector<AstNode *> &ObjectExpression::getProperties()
{
    return properties;
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

AstNode *UnaryExpression::getArgument()
{
    return argument;
}

UnaryExpression::Operator UnaryExpression::getOperator()
{
    return unaryOperator;
}

UpdateExpression::UpdateExpression(AstSourceSpan location, AstNode* argument, Operator updateOperator, bool prefix)
    : AstNode(location, AstNodeType::UpdateExpression)
    , argument{ argument }
    , updateOperator{ updateOperator }
    , prefix{ prefix }
{
    setParentOfChildren();
}

AstNode *UpdateExpression::getArgument()
{
    return argument;
}

UpdateExpression::Operator UpdateExpression::getOperator()
{
    return updateOperator;
}

bool UpdateExpression::isPrefix()
{
    return prefix;
}

BinaryExpression::BinaryExpression(AstSourceSpan location, AstNode* left, AstNode* right, Operator binaryOperator)
    : AstNode(location, AstNodeType::BinaryExpression)
    , left{ left }
    , right{ right }
    , binaryOperator{ binaryOperator }
{
    setParentOfChildren();
}

AstNode *BinaryExpression::getLeft()
{
    return left;
}

AstNode *BinaryExpression::getRight()
{
    return right;
}

BinaryExpression::Operator BinaryExpression::getOperator()
{
    return binaryOperator;
}

AssignmentExpression::AssignmentExpression(AstSourceSpan location, AstNode* left, AstNode* right, Operator assignmentOperator)
    : AstNode(location, AstNodeType::AssignmentExpression)
    , left{ left }
    , right{ right }
    , assignmentOperator{ assignmentOperator }
{
    setParentOfChildren();
}

AstNode *AssignmentExpression::getLeft()
{
    return left;
}

AstNode *AssignmentExpression::getRight()
{
    return right;
}

AssignmentExpression::Operator AssignmentExpression::getOperator()
{
    return assignmentOperator;
}

LogicalExpression::LogicalExpression(AstSourceSpan location, AstNode* left, AstNode* right, Operator logicalOperator)
    : AstNode(location, AstNodeType::LogicalExpression)
    , left{ left }
    , right{ right }
    , logicalOperator{ logicalOperator }
{
    setParentOfChildren();
}

AstNode *LogicalExpression::getLeft()
{
    return left;
}

AstNode *LogicalExpression::getRight()
{
    return right;
}

LogicalExpression::Operator LogicalExpression::getOperator()
{
    return logicalOperator;
}

MemberExpression::MemberExpression(AstSourceSpan location, AstNode* object, AstNode* property, bool computed)
    : AstNode(location, AstNodeType::MemberExpression)
    , object{ object }
    , property{ property }
    , computed{ computed }
{
    setParentOfChildren();
}

AstNode *MemberExpression::getObject()
{
    return object;
}

AstNode *MemberExpression::getProperty()
{
    return property;
}

bool MemberExpression::isComputed()
{
    return computed;
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

AstNode *ConditionalExpression::getTest()
{
    return test;
}

AstNode *ConditionalExpression::getAlternate()
{
    return alternate;
}

AstNode *ConditionalExpression::getConsequent()
{
    return consequent;
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

AstNode *CallExpression::getCallee()
{
    return callee;
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

ClassBody *Class::getBody()
{
    return reinterpret_cast<ClassBody*>(body);
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

const std::vector<AstNode *> &ClassBody::getBody()
{
    return body;
}

ClassBaseMethod::ClassBaseMethod(AstNodeType type, AstSourceSpan location, AstNode *id, std::vector<AstNode *> params,
                                 AstNode *body, AstNode *key, TypeParameterDeclaration *typeParameters, TypeAnnotation *returnType,
                                 bool isGenerator, bool isAsync, bool isComputed, bool isStatic)
    : Function(location, type, id, move(params), body, typeParameters, returnType, isGenerator, isAsync)
    , key{ key }
    , computed{ isComputed }
    , staticMethod{ isStatic }
{
}

AstNode *ClassBaseMethod::getKey()
{
    return key;
}

bool ClassBaseMethod::isComputed()
{
    return computed;
}

bool ClassBaseMethod::isStatic()
{
    return staticMethod;
}

ClassMethod::ClassMethod(AstSourceSpan location, AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key,
                         TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType,
                         ClassMethod::Kind kind, bool isGenerator, bool isAsync, bool isComputed, bool isStatic)
    : ClassBaseMethod(AstNodeType::ClassMethod, location, id, move(params), body, key, typeParameters, returnType, isGenerator, isAsync, isComputed, isStatic)
    , kind{ kind }
{
    setParentOfChildren();
}

ClassMethod::Kind ClassMethod::getKind()
{
    return kind;
}

ClassPrivateMethod::ClassPrivateMethod(AstSourceSpan location, AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key,
                                       TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType,
                                       ClassPrivateMethod::Kind kind, bool isGenerator, bool isAsync, bool isStatic)
    : ClassBaseMethod(AstNodeType::ClassPrivateMethod, location, id, move(params), body, key, typeParameters, returnType, isGenerator, isAsync, false, isStatic)
    , kind{ kind }
{
    setParentOfChildren();
}

ClassPrivateMethod::Kind ClassPrivateMethod::getKind()
{
    return kind;
}

ClassBaseProperty::ClassBaseProperty(AstNodeType type, AstSourceSpan location, AstNode *key, AstNode *value,
                                     TypeAnnotation *typeAnnotation, bool isStatic, bool isComputed)
    : AstNode(location, type)
    , key{ key }
    , value{ value }
    , typeAnnotation{ typeAnnotation }
    , staticProp{ isStatic }
    , computed{ isComputed }
{
}

bool ClassBaseProperty::isStatic()
{
    return staticProp;
}

bool ClassBaseProperty::isComputed()
{
    return computed;
}

AstNode* ClassBaseProperty::getKey()
{
    return key;
}

AstNode *ClassBaseProperty::getValue()
{
    return value;
}

TypeAnnotation *ClassBaseProperty::getTypeAnnotation()
{
    return typeAnnotation;
}

ClassProperty::ClassProperty(AstSourceSpan location, AstNode* key, AstNode* value,
                             TypeAnnotation* typeAnnotation, bool isStatic, bool isComputed)
    : ClassBaseProperty{AstNodeType::ClassProperty, location, key, value, typeAnnotation, isStatic, isComputed}
{
    setParentOfChildren();
}

ClassPrivateProperty::ClassPrivateProperty(AstSourceSpan location, AstNode* key, AstNode* value,
                                           TypeAnnotation* typeAnnotation, bool isStatic)
    : ClassBaseProperty{AstNodeType::ClassPrivateProperty, location, key, value, typeAnnotation, isStatic, false}
{
    setParentOfChildren();
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

VariableDeclaration::VariableDeclaration(AstSourceSpan location, vector<VariableDeclarator*> declarators, Kind kind)
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

const std::vector<VariableDeclarator*>& VariableDeclaration::getDeclarators()
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

AstNode *VariableDeclarator::getId()
{
    return id;
}

AstNode *VariableDeclarator::getInit()
{
    return init;
}

SpreadElement::SpreadElement(AstSourceSpan location, AstNode* argument)
    : AstNode(location, AstNodeType::SpreadElement)
    , argument{ argument }
{
    setParentOfChildren();
}

AstNode *SpreadElement::getArgument()
{
    return argument;
}

ObjectPattern::ObjectPattern(AstSourceSpan location, std::vector<AstNode*> properties, TypeAnnotation* typeAnnotation)
    : AstNode(location, AstNodeType::ObjectPattern)
    , properties{ move(properties) }
    , typeAnnotation{ typeAnnotation }
{
    setParentOfChildren();
}

const std::vector<AstNode *> &ObjectPattern::getProperties()
{
    return properties;
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

AstNode *AssignmentPattern::getLeft()
{
    return left;
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

ImportBaseSpecifier::ImportBaseSpecifier(AstNodeType type, AstSourceSpan location, AstNode* local, bool typeImport)
    : AstNode(location, type)
    , local{ (Identifier*)local }
    , typeImport{ typeImport }
{
    assert(local->getType() == AstNodeType::Identifier);
}

Identifier *ImportBaseSpecifier::getLocal()
{
    return local;
}

bool ImportBaseSpecifier::isTypeImport()
{
    return typeImport;
}

ImportSpecifier::ImportSpecifier(AstSourceSpan location, AstNode* local, AstNode* imported, bool typeImport)
    : ImportBaseSpecifier(AstNodeType::ImportSpecifier, location, local, typeImport)
    , imported{ (Identifier*)imported }
    , localEqualsImported{ this->local->getName() == this->imported->getName() }
{
    assert(imported->getType() == AstNodeType::Identifier);
    setParentOfChildren();
}

Identifier *ImportSpecifier::getImported()
{
    return reinterpret_cast<Identifier*>(imported);
}

ImportDefaultSpecifier::ImportDefaultSpecifier(AstSourceSpan location, AstNode* local)
    : ImportBaseSpecifier(AstNodeType::ImportDefaultSpecifier, location, local, false)
{
    setParentOfChildren();
}

ImportNamespaceSpecifier::ImportNamespaceSpecifier(AstSourceSpan location, AstNode* local)
    : ImportBaseSpecifier(AstNodeType::ImportNamespaceSpecifier, location, local, false)
{
    setParentOfChildren();
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

AstNode *ExportNamedDeclaration::getSource()
{
    return source;
}

ExportDefaultDeclaration::ExportDefaultDeclaration(AstSourceSpan location, AstNode* declaration)
    : AstNode(location, AstNodeType::ExportDefaultDeclaration)
    , declaration{ declaration }
{
    setParentOfChildren();
}

AstNode *ExportDefaultDeclaration::getDeclaration()
{
    return declaration;
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

AstNode *TypeAnnotation::getTypeAnnotation()
{
    return typeAnnotation;
}

GenericTypeAnnotation::GenericTypeAnnotation(AstSourceSpan location, AstNode *id, AstNode *typeParameters)
    : AstNode(location, AstNodeType::GenericTypeAnnotation)
    , id{ id }
    , typeParameters{ typeParameters }
{
    setParentOfChildren();
}

Identifier *GenericTypeAnnotation::getId()
{
    return reinterpret_cast<Identifier*>(id);
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

AstNode *NullableTypeAnnotation::getTypeAnnotation()
{
    return typeAnnotation;
}

ArrayTypeAnnotation::ArrayTypeAnnotation(AstSourceSpan location, AstNode* elementType)
    : AstNode(location, AstNodeType::ArrayTypeAnnotation)
    , elementType{ elementType }
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

IntersectionTypeAnnotation::IntersectionTypeAnnotation(AstSourceSpan location,std::vector<AstNode*> types)
    : AstNode(location, AstNodeType::IntersectionTypeAnnotation)
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

const std::vector<FunctionTypeParam *> &FunctionTypeAnnotation::getParams()
{
    return params;
}

FunctionTypeParam *FunctionTypeAnnotation::getRestParam()
{
    return rest;
}

AstNode *FunctionTypeAnnotation::getReturnType()
{
    return returnType;
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

AstNode *FunctionTypeParam::getTypeAnnotation()
{
    return typeAnnotation;
}

ObjectTypeAnnotation::ObjectTypeAnnotation(AstSourceSpan location, std::vector<AstNode *> properties, std::vector<ObjectTypeIndexer *> indexers, bool exact)
    : AstNode(location, AstNodeType::ObjectTypeAnnotation)
    , properties{ move(properties) }
    , indexers{ move(indexers) }
    , exact{ exact }
{
    setParentOfChildren();
}

const std::vector<AstNode *> &ObjectTypeAnnotation::getProperties()
{
    return properties;
}

bool ObjectTypeAnnotation::isExact()
{
    return exact;
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

ObjectTypeSpreadProperty::ObjectTypeSpreadProperty(AstSourceSpan location, AstNode *argument)
    : AstNode(location, AstNodeType::ObjectTypeSpreadProperty)
    , argument{ argument }
{
    setParentOfChildren();
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

TypeParameterDeclaration::TypeParameterDeclaration(AstSourceSpan location, std::vector<TypeParameter *> params)
    : AstNode(location, AstNodeType::TypeParameterDeclaration)
    , params{ move(params) }
{
    setParentOfChildren();
}

const std::vector<TypeParameter *> &TypeParameterDeclaration::getParams()
{
    return params;
}

TypeParameter::TypeParameter(AstSourceSpan location, std::string name, AstNode *bound)
    : AstNode(location, AstNodeType::TypeParameter)
    , name{ new Identifier(location, move(name), nullptr, false) }
    , bound{ bound }
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

AstNode *TypeCastExpression::getExpression()
{
    return expression;
}

TypeAnnotation *TypeCastExpression::getTypeAnnotation()
{
    return typeAnnotation;
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

TypeParameterDeclaration *InterfaceDeclaration::getTypeParameters()
{
    return typeParameters;
}

AstNode *InterfaceDeclaration::getBody()
{
    return body;
}

const std::vector<InterfaceExtends *> &InterfaceDeclaration::getExtends()
{
    return extends;
}

const std::vector<InterfaceExtends *> &InterfaceDeclaration::getMixins()
{
    return mixins;
}

InterfaceExtends::InterfaceExtends(AstSourceSpan location, Identifier *id, TypeParameterInstantiation* typeParameters)
    : AstNode(location, AstNodeType::InterfaceExtends)
    , id{ id }
    , typeParameters{ typeParameters }
{
    setParentOfChildren();
}

DeclareVariable::DeclareVariable(AstSourceSpan location, Identifier *id)
    : AstNode(location, AstNodeType::DeclareVariable)
    , id{ id }
{
    setParentOfChildren();
}

DeclareFunction::DeclareFunction(AstSourceSpan location, Identifier *id)
    : AstNode(location, AstNodeType::DeclareFunction)
    , id{ id }
{
    setParentOfChildren();
}

DeclareTypeAlias::DeclareTypeAlias(AstSourceSpan location, Identifier *id, AstNode* right)
    : AstNode(location, AstNodeType::DeclareTypeAlias)
    , id{ id }
    , right{ right }
{
    setParentOfChildren();
}

DeclareClass::DeclareClass(AstSourceSpan location, Identifier *id, TypeParameterDeclaration *typeParameters,
                           AstNode *body, std::vector<InterfaceExtends *> extends, std::vector<InterfaceExtends *> mixins)
    : AstNode(location, AstNodeType::DeclareClass)
    , id{ id }
    , typeParameters{ typeParameters }
    , body{ body }
    , extends{ move(extends) }
    , mixins{ move(mixins) }
{
    setParentOfChildren();
}

DeclareModule::DeclareModule(AstSourceSpan location, StringLiteral *id, AstNode *body)
    : AstNode(location, AstNodeType::DeclareModule)
    , id{ id }
    , body{ body }
{
    setParentOfChildren();
}

DeclareExportDeclaration::DeclareExportDeclaration(AstSourceSpan location, AstNode *declaration)
    : AstNode(location, AstNodeType::DeclareExportDeclaration)
    , declaration{ declaration }
{
    setParentOfChildren();
}
