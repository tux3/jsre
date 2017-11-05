#ifndef AST_HPP
#define AST_HPP

#include "import.hpp"
#include <string>
#include <vector>

#define X(IMPORTED_NODE_TYPE) IMPORTED_NODE_TYPE,
enum class AstNodeType {
    Root,
    IMPORTED_NODE_LIST(X)
};
#undef X

class AstNode {
public:
    AstNode(AstNodeType type);
    AstNodeType getType();

private:
    AstNodeType type;
};

class AstRoot : public AstNode {
public:
    AstRoot(std::vector<AstNode*> body = {});
    const std::vector<AstNode*>& getBody();

private:
    std::vector<AstNode*> body;
};

class Identifier : public AstNode {
public:
    Identifier(std::string name);

private:
    std::string name;
};

class RegExpLiteral : public AstNode {
public:
    RegExpLiteral(std::string pattern, std::string flags);

private:
    std::string pattern, flags;
};

class NullLiteral : public AstNode {
public:
    NullLiteral();
};

class StringLiteral : public AstNode {
public:
    StringLiteral(std::string value);

private:
    std::string value;
};

class BooleanLiteral : public AstNode {
public:
    BooleanLiteral(bool value);

private:
    bool value;
};

class NumericLiteral : public AstNode {
public:
    NumericLiteral(double value);

private:
    double value;
};

class TemplateLiteral : public AstNode {
public:
    TemplateLiteral(std::vector<AstNode*> quasis, std::vector<AstNode*> expressions);

private:
    std::vector<AstNode*> quasis, expressions;
};

class TemplateElement : public AstNode {
public:
    TemplateElement(std::string rawValue, bool isTail);

private:
    std::string rawValue;
    bool isTail;
};

class TaggedTemplateExpression : public AstNode {
public:
    TaggedTemplateExpression(AstNode* tag, AstNode* quasi);

private:
    AstNode *tag, *quasi;
};

class Function : public AstNode {
protected:
    Function(AstNodeType type, AstNode* id, std::vector<AstNode*> params, AstNode* body, bool isGenerator, bool isAsync);

protected:
    AstNode* id;
    std::vector<AstNode*> params;
    AstNode* body;
    bool isGenerator, isAsync;
};

class ObjectProperty : public AstNode {
public:
    ObjectProperty(AstNode* key, AstNode* value, bool isShorthand, bool isComputed);

private:
    AstNode *key, *value;
    bool isShorthand, isComputed;
};

class ObjectMethod : public Function {
public:
    enum class Kind {
        Constructor,
        Method,
        Get,
        Set,
    };

    ObjectMethod(AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key, Kind kind,
        bool isGenerator, bool isAsync, bool isComputed);

private:
    AstNode* key;
    Kind kind;
    bool isComputed;
};

class ExpressionStatement : public AstNode {
public:
    ExpressionStatement(AstNode* expression);

private:
    AstNode* expression;
};

class BlockStatement : public AstNode {
public:
    BlockStatement(std::vector<AstNode*> body);

private:
    std::vector<AstNode*> body;
};

class EmptyStatement : public AstNode {
public:
    EmptyStatement();
};

class WithStatement : public AstNode {
public:
    WithStatement(AstNode* object, AstNode* body);

private:
    AstNode *object, *body;
};

class DebuggerStatement : public AstNode {
public:
    DebuggerStatement();
};

class ReturnStatement : public AstNode {
public:
    ReturnStatement(AstNode* argument);

private:
    AstNode* argument;
};

class LabeledStatement : public AstNode {
public:
    LabeledStatement(AstNode* label, AstNode* body);

private:
    AstNode *label, *body;
};

class BreakStatement : public AstNode {
public:
    BreakStatement(AstNode* label);

private:
    AstNode* label;
};

class ContinueStatement : public AstNode {
public:
    ContinueStatement(AstNode* label);

private:
    AstNode* label;
};

class IfStatement : public AstNode {
public:
    IfStatement(AstNode* test, AstNode* consequent, AstNode* argument);

private:
    AstNode *test, *consequent, *argument;
};

class SwitchStatement : public AstNode {
public:
    SwitchStatement(AstNode* discriminant, std::vector<AstNode*> cases);

private:
    AstNode* discriminant;
    std::vector<AstNode*> cases;
};

class SwitchCase : public AstNode {
public:
    SwitchCase(AstNode* testOrDefault, std::vector<AstNode*> consequent);

private:
    AstNode* testOrDefault;
    std::vector<AstNode*> consequent;
};

class ThrowStatement : public AstNode {
public:
    ThrowStatement(AstNode* argument);

private:
    AstNode* argument;
};

class TryStatement : public AstNode {
public:
    TryStatement(AstNode* body, AstNode* handler, AstNode* finalizer);

private:
    AstNode *body, *handler, *finalizer;
};

class CatchClause : public AstNode {
public:
    CatchClause(AstNode* param, AstNode* body);

private:
    AstNode *param, *body;
};

class WhileStatement : public AstNode {
public:
    WhileStatement(AstNode* test, AstNode* body);

private:
    AstNode *test, *body;
};

class DoWhileStatement : public AstNode {
public:
    DoWhileStatement(AstNode* test, AstNode* body);

private:
    AstNode *test, *body;
};

class ForStatement : public AstNode {
public:
    ForStatement(AstNode* init, AstNode* test, AstNode* update, AstNode* body);

private:
    AstNode *init, *test, *update, *body;
};

class ForInStatement : public AstNode {
public:
    ForInStatement(AstNode* left, AstNode* right, AstNode* body);

private:
    AstNode *left, *right, *body;
};

class ForOfStatement : public AstNode {
public:
    ForOfStatement(AstNode* left, AstNode* right, AstNode* body, bool isAwait);

private:
    AstNode *left, *right, *body;
    bool isAwait;
};

class Super : public AstNode {
public:
    Super();
};

class Import : public AstNode {
public:
    Import();
};

class ThisExpression : public AstNode {
public:
    ThisExpression();
};

class ArrowFunctionExpression : public Function {
public:
    ArrowFunctionExpression(AstNode* id, std::vector<AstNode*> params, AstNode* body, bool isGenerator, bool isAsync, bool isExpression);

private:
    bool isExpression;
};

class YieldExpression : public AstNode {
public:
    YieldExpression(AstNode* argument, bool isDelegate);

private:
    AstNode* argument;
    bool isDelegate;
};

class AwaitExpression : public AstNode {
public:
    AwaitExpression(AstNode* argument);

private:
    AstNode* argument;
};

class ArrayExpression : public AstNode {
public:
    ArrayExpression(std::vector<AstNode*> elements);

private:
    std::vector<AstNode*> elements;
};

class ObjectExpression : public AstNode {
public:
    ObjectExpression(std::vector<AstNode*> properties);

private:
    std::vector<AstNode*> properties;
};

class FunctionExpression : public Function {
public:
    FunctionExpression(AstNode* id, std::vector<AstNode*> params, AstNode* body, bool isGenerator, bool isAsync);
};

class UnaryExpression : public AstNode {
public:
    enum class Operator {
        Minus,
        Plus,
        LogicalNot,
        BitwiseNot,
        Typeof,
        Void,
        Delete,
        Throw,
    };
    UnaryExpression(AstNode* argument, Operator unaryOperator, bool isPrefix);

private:
    AstNode* argument;
    Operator unaryOperator;
    bool isPrefix;
};

class UpdateExpression : public AstNode {
public:
    enum class Operator {
        Increment,
        Decrement,
    };
    UpdateExpression(AstNode* argument, Operator updateOperator, bool isPrefix);

private:
    AstNode* argument;
    Operator updateOperator;
    bool isPrefix;
};

class BinaryExpression : public AstNode {
public:
    enum class Operator {
        Equal,
        NotEqual,
        StrictEqual,
        StrictNotEqual,
        Lesser,
        LesserOrEqual,
        Greater,
        GreaterOrEqual,
        ShiftLeft,
        SignShiftRight,
        ZeroingShiftRight,
        Plus,
        Minus,
        Times,
        Division,
        Modulo,
        BitwiseOr,
        BitwiseXor,
        BitwiseAnd,
        In,
        Instanceof,
    };
    BinaryExpression(AstNode* left, AstNode* right, Operator binaryOperator);

private:
    AstNode *left, *right;
    Operator binaryOperator;
};

class AssignmentExpression : public AstNode {
public:
    enum class Operator {
        Equal,
        PlusEqual,
        MinusEqual,
        TimesEqual,
        SlashEqual,
        ModuloEqual,
        LeftShiftEqual,
        SignRightShiftEqual,
        ZeroingRightShiftEqual,
        OrEqual,
        XorEqual,
        AndEqual,
    };
    AssignmentExpression(AstNode* left, AstNode* right, Operator assignmentOperator);

private:
    AstNode *left, *right;
    Operator assignmentOperator;
};

class LogicalExpression : public AstNode {
public:
    enum class Operator {
        Or,
        And,
    };
    LogicalExpression(AstNode* left, AstNode* right, Operator logicalOperator);

private:
    AstNode *left, *right;
    Operator logicalOperator;
};

class MemberExpression : public AstNode {
public:
    MemberExpression(AstNode* object, AstNode* property, bool isComputed);

private:
    AstNode *object, *property;
    bool isComputed;
};

class BindExpression : public AstNode {
public:
    BindExpression(AstNode* object, AstNode* callee);

private:
    AstNode *object, *callee;
};

class ConditionalExpression : public AstNode {
public:
    ConditionalExpression(AstNode* test, AstNode* alternate, AstNode* consequent);

private:
    AstNode *test, *alternate, *consequent;
};

class CallExpression : public AstNode {
public:
    CallExpression(AstNode* callee, std::vector<AstNode*> arguments);

protected:
    CallExpression(AstNodeType type, AstNode* callee, std::vector<AstNode*> arguments);

private:
    AstNode* callee;
    std::vector<AstNode*> arguments;
};

class NewExpression : public CallExpression {
public:
    NewExpression(AstNode* callee, std::vector<AstNode*> arguments);
};

class SequenceExpression : public AstNode {
public:
    SequenceExpression(std::vector<AstNode*> expressions);

private:
    std::vector<AstNode*> expressions;
};

class DoExpression : public AstNode {
public:
    DoExpression(AstNode* body);

private:
    AstNode* body;
};

class Class : public AstNode {
protected:
    Class(AstNodeType type, AstNode* id, AstNode* superClass, AstNode* body);

protected:
    AstNode *id, *superClass;
    AstNode* body;
};

class ClassExpression : public Class {
public:
    ClassExpression(AstNode* id, AstNode* superClass, AstNode* body);
};

class ClassDeclaration : public Class {
public:
    ClassDeclaration(AstNode* id, AstNode* superClass, AstNode* body);
};

class ClassBody : public AstNode {
public:
    ClassBody(std::vector<AstNode*> body);

private:
    std::vector<AstNode*> body;
};

class ClassProperty : public AstNode {
public:
    ClassProperty(AstNode* key, AstNode* value, bool isStatic, bool isComputed);

private:
    AstNode *key, *value;
    bool isStatic, isComputed;
};

class ClassPrivateProperty : public AstNode {
public:
    ClassPrivateProperty(AstNode* key, AstNode* value, bool isStatic);

private:
    AstNode *key, *value;
    bool isStatic;
};

class ClassMethod : public Function {
public:
    enum class Kind {
        Constructor,
        Method,
        Get,
        Set,
    };

    ClassMethod(AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key, Kind kind,
        bool isGenerator, bool isAsync, bool isComputed, bool isStatic);

private:
    AstNode* key;
    Kind kind;
    bool isComputed, isStatic;
};

class ClassPrivateMethod : public Function {
public:
    enum class Kind {
        Method,
        Get,
        Set,
    };

    ClassPrivateMethod(AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key, Kind kind,
        bool isGenerator, bool isAsync, bool isStatic);

private:
    AstNode* key;
    Kind kind;
    bool isStatic;
};

class FunctionDeclaration : public Function {
public:
    FunctionDeclaration(AstNode* id, std::vector<AstNode*> params, AstNode* body, bool isGenerator, bool isAsync);
};

class VariableDeclaration : public AstNode {
public:
    enum class Kind {
        Var,
        Let,
        Const
    };
    VariableDeclaration(std::vector<AstNode*> declarators, Kind kind);

private:
    std::vector<AstNode*> declarators;
    Kind kind;
};

class VariableDeclarator : public AstNode {
public:
    VariableDeclarator(AstNode* id, AstNode* init);

private:
    AstNode *id, *init;
};

class SpreadElement : public AstNode {
public:
    SpreadElement(AstNode* argument);

private:
    AstNode* argument;
};

class ObjectPattern : public AstNode {
public:
    ObjectPattern(std::vector<AstNode*> properties);

private:
    std::vector<AstNode*> properties;
};

class ArrayPattern : public AstNode {
public:
    ArrayPattern(std::vector<AstNode*> elements);

private:
    std::vector<AstNode*> elements;
};

class AssignmentPattern : public AstNode {
public:
    AssignmentPattern(AstNode* left, AstNode* right);

private:
    AstNode *left, *right;
};

class RestElement : public AstNode {
public:
    RestElement(AstNode* argument);

private:
    AstNode* argument;
};

class MetaProperty : public AstNode {
public:
    MetaProperty(AstNode* meta, AstNode* property);

private:
    AstNode *meta, *property;
};

class ImportDeclaration : public AstNode {
public:
    ImportDeclaration(std::vector<AstNode*> specifiers, AstNode* source);

private:
    std::vector<AstNode*> specifiers;
    AstNode* source;
};

class ImportSpecifier : public AstNode {
public:
    ImportSpecifier(AstNode* local, AstNode* imported);

private:
    AstNode *local, *imported;
};

class ImportDefaultSpecifier : public AstNode {
public:
    ImportDefaultSpecifier(AstNode* local);

private:
    AstNode* local;
};

class ImportNamespaceSpecifier : public AstNode {
public:
    ImportNamespaceSpecifier(AstNode* local);

private:
    AstNode* local;
};

class ExportNamedDeclaration : public AstNode {
public:
    ExportNamedDeclaration(AstNode* declaration, AstNode* source, std::vector<AstNode*> specifiers);

private:
    AstNode *declaration, *source;
    std::vector<AstNode*> specifiers;
};

class ExportDefaultDeclaration : public AstNode {
public:
    ExportDefaultDeclaration(AstNode* declaration);

private:
    AstNode* declaration;
};

class ExportAllDeclaration : public AstNode {
public:
    ExportAllDeclaration(AstNode* source);

private:
    AstNode* source;
};

class ExportSpecifier : public AstNode {
public:
    ExportSpecifier(AstNode* local, AstNode* exported);

private:
    AstNode *local, *exported;
};

#endif // AST_HPP
