#include "ast.hpp"

using namespace std;

AstNode::AstNode(AstNodeType type)
    : parent{ nullptr }, type{ type }
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

void AstNode::setParentOfChildren() {
    for (auto child : getChildren())
        if (child)
            child->parent = this;
}

AstRoot::AstRoot(Module &parentModule, vector<AstNode*> body)
    : AstNode(AstNodeType::Root)
    , parentModule{ parentModule }
    , body{ body }
{
    setParentOfChildren();
}

const std::vector<AstNode*>& AstRoot::getBody() {
    return body;
}

Identifier::Identifier(string name)
    : AstNode(AstNodeType::Identifier)
    , name{ name }
{
    setParentOfChildren();
}

const std::string& Identifier::getName()
{
    return name;
}

RegExpLiteral::RegExpLiteral(string pattern, string flags)
    : AstNode(AstNodeType::RegExpLiteral)
    , pattern{ pattern }
    , flags{ flags }
{
    setParentOfChildren();
}

NullLiteral::NullLiteral()
    : AstNode(AstNodeType::NullLiteral)
{
    setParentOfChildren();
}

StringLiteral::StringLiteral(string value)
    : AstNode(AstNodeType::StringLiteral)
    , value{ value }
{
    setParentOfChildren();
}

const string &StringLiteral::getValue()
{
    return value;
}

BooleanLiteral::BooleanLiteral(bool value)
    : AstNode(AstNodeType::BooleanLiteral)
    , value{ value }
{
    setParentOfChildren();
}

bool BooleanLiteral::getValue()
{
    return value;
}

NumericLiteral::NumericLiteral(double value)
    : AstNode(AstNodeType::NumericLiteral)
    , value{ value }
{
    setParentOfChildren();
}

double NumericLiteral::getValue()
{
    return value;
}

TemplateLiteral::TemplateLiteral(std::vector<AstNode*> quasis, std::vector<AstNode*> expressions)
    : AstNode(AstNodeType::TemplateLiteral)
    , quasis{ quasis }
    , expressions{ expressions }
{
    setParentOfChildren();
}

TemplateElement::TemplateElement(std::string rawValue, bool isTail)
    : AstNode(AstNodeType::TemplateElement)
    , rawValue{ rawValue }
    , isTail{ isTail }
{
    setParentOfChildren();
}

TaggedTemplateExpression::TaggedTemplateExpression(AstNode* tag, AstNode* quasi)
    : AstNode(AstNodeType::TaggedTemplateExpression)
    , tag{ tag }
    , quasi{ quasi }
{
    setParentOfChildren();
}

ObjectProperty::ObjectProperty(AstNode* key, AstNode* value, bool isShorthand, bool isComputed)
    : AstNode(AstNodeType::ObjectProperty)
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

ObjectMethod::ObjectMethod(AstNode* id, vector<AstNode*> params, AstNode* body, AstNode* key, ObjectMethod::Kind kind,
    bool isGenerator, bool isAsync, bool isComputed)
    : Function(AstNodeType::ObjectMethod, id, params, body, isGenerator, isAsync)
    , key{ key }
    , kind{ kind }
    , isComputed{ isComputed }
{
    setParentOfChildren();
}

ExpressionStatement::ExpressionStatement(AstNode* expression)
    : AstNode(AstNodeType::ExpressionStatement)
    , expression{ expression }
{
    setParentOfChildren();
}

BlockStatement::BlockStatement(std::vector<AstNode*> body)
    : AstNode(AstNodeType::BlockStatement)
    , body{ body }
{
    setParentOfChildren();
}

EmptyStatement::EmptyStatement()
    : AstNode(AstNodeType::EmptyStatement)
{
    setParentOfChildren();
}

WithStatement::WithStatement(AstNode* object, AstNode* body)
    : AstNode(AstNodeType::WithStatement)
    , object{ object }
    , body{ body }
{
    setParentOfChildren();
}

DebuggerStatement::DebuggerStatement()
    : AstNode(AstNodeType::DebuggerStatement)
{
    setParentOfChildren();
}

ReturnStatement::ReturnStatement(AstNode* argument)
    : AstNode(AstNodeType::ReturnStatement)
    , argument{ argument }
{
    setParentOfChildren();
}

LabeledStatement::LabeledStatement(AstNode* label, AstNode* body)
    : AstNode(AstNodeType::LabeledStatement)
    , label{ label }
    , body{ body }
{
    setParentOfChildren();
}

BreakStatement::BreakStatement(AstNode* label)
    : AstNode(AstNodeType::BreakStatement)
    , label{ label }
{
    setParentOfChildren();
}

ContinueStatement::ContinueStatement(AstNode* label)
    : AstNode(AstNodeType::ContinueStatement)
    , label{ label }
{
    setParentOfChildren();
}

IfStatement::IfStatement(AstNode* test, AstNode* consequent, AstNode* argument)
    : AstNode(AstNodeType::IfStatement)
    , test{ test }
    , consequent{ consequent }
    , argument{ argument }
{
    setParentOfChildren();
}

SwitchStatement::SwitchStatement(AstNode* discriminant, std::vector<AstNode*> cases)
    : AstNode(AstNodeType::SwitchStatement)
    , discriminant{ discriminant }
    , cases{ cases }
{
    setParentOfChildren();
}

SwitchCase::SwitchCase(AstNode* testOrDefault, std::vector<AstNode*> consequent)
    : AstNode(AstNodeType::SwitchCase)
    , testOrDefault{ testOrDefault }
    , consequent{ consequent }
{
    setParentOfChildren();
}

const std::vector<AstNode *> &SwitchCase::getConsequent()
{
    return consequent;
}

ThrowStatement::ThrowStatement(AstNode* argument)
    : AstNode(AstNodeType::ThrowStatement)
    , argument{ argument }
{
    setParentOfChildren();
}

TryStatement::TryStatement(AstNode* body, AstNode* handler, AstNode* finalizer)
    : AstNode(AstNodeType::TryStatement)
    , body{ body }
    , handler{ handler }
    , finalizer{ finalizer }
{
    setParentOfChildren();
}

CatchClause::CatchClause(AstNode* param, AstNode* body)
    : AstNode(AstNodeType::CatchClause)
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

WhileStatement::WhileStatement(AstNode* test, AstNode* body)
    : AstNode(AstNodeType::WhileStatement)
    , test{ test }
    , body{ body }
{
    setParentOfChildren();
}

DoWhileStatement::DoWhileStatement(AstNode* test, AstNode* body)
    : AstNode(AstNodeType::DoWhileStatement)
    , test{ test }
    , body{ body }
{
    setParentOfChildren();
}

ForStatement::ForStatement(AstNode* init, AstNode* test, AstNode* update, AstNode* body)
    : AstNode(AstNodeType::ForStatement)
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

ForInStatement::ForInStatement(AstNode* left, AstNode* right, AstNode* body)
    : AstNode(AstNodeType::ForInStatement)
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

ForOfStatement::ForOfStatement(AstNode* left, AstNode* right, AstNode* body, bool isAwait)
    : AstNode(AstNodeType::ForOfStatement)
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

Function::Function(AstNodeType type, AstNode* id, std::vector<AstNode*> params, AstNode* body, bool generator, bool async)
    : AstNode(type)
    , id{ id }
    , params{ params }
    , body{ body }
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

Super::Super()
    : AstNode(AstNodeType::Super)
{
    setParentOfChildren();
}

Import::Import()
    : AstNode(AstNodeType::Import)
{
    setParentOfChildren();
}

ThisExpression::ThisExpression()
    : AstNode(AstNodeType::ThisExpression)
{
    setParentOfChildren();
}

ArrowFunctionExpression::ArrowFunctionExpression(AstNode* id, vector<AstNode*> params, AstNode* body, bool isGenerator, bool isAsync, bool isExpression)
    : Function(AstNodeType::ArrowFunctionExpression, id, params, body, isGenerator, isAsync)
    , isExpression{ isExpression }
{
    setParentOfChildren();
}

YieldExpression::YieldExpression(AstNode* argument, bool isDelegate)
    : AstNode(AstNodeType::YieldExpression)
    , argument{ argument }
    , isDelegate{ isDelegate }
{
    setParentOfChildren();
}

AwaitExpression::AwaitExpression(AstNode* argument)
    : AstNode(AstNodeType::AwaitExpression)
    , argument{ argument }
{
    setParentOfChildren();
}

ArrayExpression::ArrayExpression(vector<AstNode*> elements)
    : AstNode(AstNodeType::ArrayExpression)
    , elements{ elements }
{
    setParentOfChildren();
}

ObjectExpression::ObjectExpression(vector<AstNode*> properties)
    : AstNode(AstNodeType::ObjectExpression)
    , properties{ properties }
{
    setParentOfChildren();
}

FunctionExpression::FunctionExpression(AstNode* id, vector<AstNode*> params, AstNode* body, bool isGenerator, bool isAsync)
    : Function(AstNodeType::FunctionExpression, id, params, body, isGenerator, isAsync)
{
    setParentOfChildren();
}

UnaryExpression::UnaryExpression(AstNode* argument, Operator unaryOperator, bool isPrefix)
    : AstNode(AstNodeType::UnaryExpression)
    , argument{ argument }
    , unaryOperator{ unaryOperator }
    , isPrefix{ isPrefix }
{
    setParentOfChildren();
}

UpdateExpression::UpdateExpression(AstNode* argument, Operator updateOperator, bool isPrefix)
    : AstNode(AstNodeType::UpdateExpression)
    , argument{ argument }
    , updateOperator{ updateOperator }
    , isPrefix{ isPrefix }
{
    setParentOfChildren();
}

BinaryExpression::BinaryExpression(AstNode* left, AstNode* right, Operator binaryOperator)
    : AstNode(AstNodeType::BinaryExpression)
    , left{ left }
    , right{ right }
    , binaryOperator{ binaryOperator }
{
    setParentOfChildren();
}

AssignmentExpression::AssignmentExpression(AstNode* left, AstNode* right, Operator assignmentOperator)
    : AstNode(AstNodeType::AssignmentExpression)
    , left{ left }
    , right{ right }
    , assignmentOperator{ assignmentOperator }
{
    setParentOfChildren();
}

LogicalExpression::LogicalExpression(AstNode* left, AstNode* right, Operator logicalOperator)
    : AstNode(AstNodeType::LogicalExpression)
    , left{ left }
    , right{ right }
    , logicalOperator{ logicalOperator }
{
    setParentOfChildren();
}

MemberExpression::MemberExpression(AstNode* object, AstNode* property, bool isComputed)
    : AstNode(AstNodeType::MemberExpression)
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

BindExpression::BindExpression(AstNode* object, AstNode* callee)
    : AstNode(AstNodeType::BindExpression)
    , object{ object }
    , callee{ callee }
{
    setParentOfChildren();
}

ConditionalExpression::ConditionalExpression(AstNode* test, AstNode* alternate, AstNode* consequent)
    : AstNode(AstNodeType::ConditionalExpression)
    , test{ test }
    , alternate{ alternate }
    , consequent{ consequent }
{
    setParentOfChildren();
}

CallExpression::CallExpression(AstNode* callee, vector<AstNode*> arguments)
    : CallExpression(AstNodeType::CallExpression, callee, arguments)
{
    setParentOfChildren();
}

const std::vector<AstNode *> &CallExpression::getArguments()
{
    return arguments;
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
    setParentOfChildren();
}

SequenceExpression::SequenceExpression(std::vector<AstNode*> expressions)
    : AstNode(AstNodeType::SequenceExpression)
    , expressions{ expressions }
{
    setParentOfChildren();
}

DoExpression::DoExpression(AstNode* body)
    : AstNode(AstNodeType::DoExpression)
    , body{ body }
{
    setParentOfChildren();
}

Class::Class(AstNodeType type, AstNode* id, AstNode* superClass, AstNode* body)
    : AstNode(type)
    , id{ id }
    , superClass{ superClass }
    , body{ body }
{
}

Identifier* Class::getId()
{
    return reinterpret_cast<Identifier*>(id);
}

ClassExpression::ClassExpression(AstNode* id, AstNode* superClass, AstNode* body)
    : Class(AstNodeType::ClassExpression, id, superClass, body)
{
    setParentOfChildren();
}

ClassBody::ClassBody(std::vector<AstNode*> body)
    : AstNode(AstNodeType::ClassBody)
    , body{ body }
{
    setParentOfChildren();
}

ClassMethod::ClassMethod(AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key, ClassMethod::Kind kind,
    bool isGenerator, bool isAsync, bool isComputed, bool isStatic)
    : Function(AstNodeType::ClassMethod, id, params, body, isGenerator, isAsync)
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

ClassPrivateMethod::ClassPrivateMethod(AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key, ClassPrivateMethod::Kind kind,
    bool isGenerator, bool isAsync, bool isStatic)
    : Function(AstNodeType::ClassPrivateMethod, id, params, body, isGenerator, isAsync)
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

ClassProperty::ClassProperty(AstNode* key, AstNode* value, bool isStatic, bool isComputed)
    : AstNode(AstNodeType::ClassProperty)
    , key{ key }
    , value{ value }
    , isStatic{ isStatic }
    , isComputed{ isComputed }
{
    setParentOfChildren();
}

Identifier* ClassProperty::getKey()
{
    return reinterpret_cast<Identifier*>(key);
}

ClassPrivateProperty::ClassPrivateProperty(AstNode* key, AstNode* value, bool isStatic)
    : AstNode(AstNodeType::ClassPrivateProperty)
    , key{ key }
    , value{ value }
    , isStatic{ isStatic }
{
    setParentOfChildren();
}

Identifier* ClassPrivateProperty::getKey()
{
    return reinterpret_cast<Identifier*>(key);
}

ClassDeclaration::ClassDeclaration(AstNode* id, AstNode* superClass, AstNode* body)
    : Class(AstNodeType::ClassDeclaration, id, superClass, body)
{
    setParentOfChildren();
}

FunctionDeclaration::FunctionDeclaration(AstNode* id, vector<AstNode*> params, AstNode* body, bool isGenerator, bool isAsync)
    : Function(AstNodeType::FunctionDeclaration, id, params, body, isGenerator, isAsync)
{
    setParentOfChildren();
}

VariableDeclaration::VariableDeclaration(vector<AstNode*> declarators, Kind kind)
    : AstNode(AstNodeType::VariableDeclaration)
    , declarators{ declarators }
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

VariableDeclarator::VariableDeclarator(AstNode* id, AstNode* init)
    : AstNode(AstNodeType::VariableDeclarator)
    , id{ id }
    , init{ init }
{
    setParentOfChildren();
}

AstNode* VariableDeclarator::getId()
{
    return id;
}

SpreadElement::SpreadElement(AstNode* argument)
    : AstNode(AstNodeType::SpreadElement)
    , argument{ argument }
{
    setParentOfChildren();
}

ObjectPattern::ObjectPattern(std::vector<AstNode*> properties)
    : AstNode{ AstNodeType::ObjectPattern }
    , properties{ properties }
{
    setParentOfChildren();
}

const std::vector<ObjectProperty *> &ObjectPattern::getProperties()
{
    return reinterpret_cast<vector<ObjectProperty*>&>(properties);
}

ArrayPattern::ArrayPattern(std::vector<AstNode*> elements)
    : AstNode{ AstNodeType::ArrayPattern }
    , elements{ elements }
{
    setParentOfChildren();
}

const std::vector<AstNode*>& ArrayPattern::getElements()
{
    return elements;
}

AssignmentPattern::AssignmentPattern(AstNode* left, AstNode* right)
    : AstNode(AstNodeType::AssignmentPattern)
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

RestElement::RestElement(AstNode* argument)
    : AstNode(AstNodeType::RestElement)
    , argument{ argument }
{
    setParentOfChildren();
}

Identifier *RestElement::getArgument()
{
    return reinterpret_cast<Identifier*>(argument);
}

MetaProperty::MetaProperty(AstNode* meta, AstNode* property)
    : AstNode(AstNodeType::MetaProperty)
    , meta{ meta }
    , property{ property }
{
    setParentOfChildren();
}

ImportDeclaration::ImportDeclaration(std::vector<AstNode*> specifiers, AstNode* source)
    : AstNode(AstNodeType::ImportDeclaration)
    , specifiers{ specifiers }
    , source{ source }
{
    setParentOfChildren();
}

string ImportDeclaration::getSource()
{
    assert(source->getType() == AstNodeType::StringLiteral);
    auto sourceLiteral = (StringLiteral*)source;
    return sourceLiteral->getValue();
}

const std::vector<AstNode*>& ImportDeclaration::getSpecifiers()
{
    return specifiers;
}

ImportSpecifier::ImportSpecifier(AstNode* local, AstNode* imported)
    : AstNode(AstNodeType::ImportSpecifier)
    , local{ (Identifier*)local }
    , imported{ (Identifier*)imported }
    , localEqualsImported{ this->local->getName() == this->imported->getName() }
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

ImportDefaultSpecifier::ImportDefaultSpecifier(AstNode* local)
    : AstNode(AstNodeType::ImportDefaultSpecifier)
    , local{ local }
{
    setParentOfChildren();
}

Identifier* ImportDefaultSpecifier::getLocal()
{
    return reinterpret_cast<Identifier*>(local);
}

ImportNamespaceSpecifier::ImportNamespaceSpecifier(AstNode* local)
    : AstNode(AstNodeType::ImportNamespaceSpecifier)
    , local{ local }
{
    setParentOfChildren();
}

Identifier* ImportNamespaceSpecifier::getLocal()
{
    return reinterpret_cast<Identifier*>(local);
}

ExportNamedDeclaration::ExportNamedDeclaration(AstNode* declaration, AstNode* source, std::vector<AstNode*> specifiers)
    : AstNode(AstNodeType::ExportNamedDeclaration)
    , declaration{ declaration }
    , source{ source }
    , specifiers{ specifiers }
{
    setParentOfChildren();
}

ExportDefaultDeclaration::ExportDefaultDeclaration(AstNode* declaration)
    : AstNode(AstNodeType::ExportDefaultDeclaration)
    , declaration{ declaration }
{
    setParentOfChildren();
}

ExportAllDeclaration::ExportAllDeclaration(AstNode* source)
    : AstNode(AstNodeType::ExportAllDeclaration)
    , source{ source }
{
    setParentOfChildren();
}

ExportSpecifier::ExportSpecifier(AstNode* local, AstNode* exported)
    : AstNode(AstNodeType::ExportSpecifier)
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

ExportDefaultSpecifier::ExportDefaultSpecifier(AstNode* exported)
    : AstNode(AstNodeType::ExportDefaultSpecifier)
    , exported{ exported }
{
    setParentOfChildren();
}

Identifier *ExportDefaultSpecifier::getExported()
{
    return reinterpret_cast<Identifier*>(exported);
}
