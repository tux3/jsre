#ifndef AST_HPP
#define AST_HPP

#include "import.hpp"
#include "location.hpp"
#include <string>
#include <vector>
#include <functional>

#define X(IMPORTED_NODE_TYPE) IMPORTED_NODE_TYPE,
enum class AstNodeType {
    Root,
    IMPORTED_NODE_LIST(X)
    Invalid,
};
#undef X

#define X(IMPORTED_NODE_TYPE) class IMPORTED_NODE_TYPE;
IMPORTED_NODE_LIST(X)
#undef X

class AstNode {
public:
    AstNode(AstSourceSpan location, AstNodeType type);
    AstNode(const AstNode& other) = delete;
    virtual ~AstNode() = default;
    AstNode* getParent();
    const AstNode* getParent() const;
    Module& getParentModule() const;
    AstNodeType getType() const;
    const char* getTypeName() const;
    AstSourceSpan getLocation() const; //< Location of the AST node. Don't forget this is UTF8, not bytes!
    std::string getSourceString();
    std::vector<AstNode*> getChildren();
    virtual void applyChildren(const std::function<bool (AstNode*)>&); // Return false to stop iterating

protected:
    void setParentOfChildren();

private:
    AstNode* parent;
    AstSourceSpan location;
    AstNodeType type;
};

class AstRoot : public AstNode {
public:
    AstRoot(AstSourceSpan location, Module& parentModule, std::vector<AstNode*> body = {});
    const std::vector<AstNode*>& getBody();
    Module& getParentModule() const;
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Module& parentModule;
    std::vector<AstNode*> body;
};

class Identifier : public AstNode {
public:
    Identifier(AstSourceSpan location, std::string name, TypeAnnotation* typeAnnotation, bool optional);
    const std::string& getName();
    TypeAnnotation* getTypeAnnotation();
    bool isOptional();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::string name;
    TypeAnnotation* typeAnnotation;
    bool optional; // Flow syntax, e.g. for optional parameters
};

class RegExpLiteral : public AstNode {
public:
    RegExpLiteral(AstSourceSpan location, std::string pattern, std::string flags);
    const std::string& getPattern();

private:
    std::string pattern, flags;
};

class NullLiteral : public AstNode {
public:
    NullLiteral(AstSourceSpan location);
};

class StringLiteral : public AstNode {
public:
    StringLiteral(AstSourceSpan location, std::string value);
    const std::string& getValue();

private:
    std::string value;
};

class BooleanLiteral : public AstNode {
public:
    BooleanLiteral(AstSourceSpan location, bool value);
    bool getValue();

private:
    bool value;
};

class NumericLiteral : public AstNode {
public:
    NumericLiteral(AstSourceSpan location, double value);
    double getValue();

private:
    double value;
};

class TemplateLiteral : public AstNode {
public:
    TemplateLiteral(AstSourceSpan location, std::vector<AstNode*> quasis, std::vector<AstNode*> expressions);
    const std::vector<AstNode*>& getQuasis();
    const std::vector<AstNode*>& getExpressions();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> quasis, expressions;
};

class TemplateElement : public AstNode {
public:
    TemplateElement(AstSourceSpan location, std::string rawValue, bool tail);
    bool isTail();

private:
    std::string rawValue;
    bool tail;
};

class TaggedTemplateExpression : public AstNode {
public:
    TaggedTemplateExpression(AstSourceSpan location, AstNode* tag, AstNode* quasi);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *tag, *quasi;
};


class ExpressionStatement : public AstNode {
public:
    ExpressionStatement(AstSourceSpan location, AstNode* expression);
    AstNode* getExpression();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* expression;
};

class BlockStatement : public AstNode {
public:
    BlockStatement(AstSourceSpan location, std::vector<AstNode*> body);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> body;
};

class EmptyStatement : public AstNode {
public:
    EmptyStatement(AstSourceSpan location);
};

class WithStatement : public AstNode {
public:
    WithStatement(AstSourceSpan location, AstNode* object, AstNode* body);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *object, *body;
};

class DebuggerStatement : public AstNode {
public:
    DebuggerStatement(AstSourceSpan location);
};

class ReturnStatement : public AstNode {
public:
    ReturnStatement(AstSourceSpan location, AstNode* argument);
    AstNode* getArgument();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* argument;
};

class LabeledStatement : public AstNode {
public:
    LabeledStatement(AstSourceSpan location, AstNode* label, AstNode* body);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *label, *body;
};

class BreakStatement : public AstNode {
public:
    BreakStatement(AstSourceSpan location, AstNode* label);
    AstNode* getLabel();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* label;
};

class ContinueStatement : public AstNode {
public:
    ContinueStatement(AstSourceSpan location, AstNode* label);
    AstNode* getLabel();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* label;
};

class IfStatement : public AstNode {
public:
    IfStatement(AstSourceSpan location, AstNode* test, AstNode* consequent, AstNode* alternate);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;
    AstNode* getTest();
    AstNode* getConsequent();
    AstNode* getAlternate();

private:
    AstNode *test, *consequent, *alternate;
};

class SwitchStatement : public AstNode {
public:
    SwitchStatement(AstSourceSpan location, AstNode* discriminant, std::vector<SwitchCase*> cases);
    AstNode* getDiscriminant();
    const std::vector<SwitchCase*>& getCases();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* discriminant;
    std::vector<SwitchCase*> cases;
};

class SwitchCase : public AstNode {
public:
    SwitchCase(AstSourceSpan location, AstNode* testOrDefault, std::vector<AstNode*> consequent);
    AstNode* getTest();
    const std::vector<AstNode*>& getConsequents();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* testOrDefault; // Is a nullptr for the default case
    std::vector<AstNode*> consequent;
};

class ThrowStatement : public AstNode {
public:
    ThrowStatement(AstSourceSpan location, AstNode* argument);
    AstNode* getArgument();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* argument;
};

class TryStatement : public AstNode {
public:
    TryStatement(AstSourceSpan location, AstNode* block, AstNode* handler, AstNode* finalizer);
    AstNode* getBlock();
    CatchClause* getHandler();
    AstNode* getFinalizer();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *block, *handler, *finalizer;
};

class CatchClause : public AstNode {
public:
    CatchClause(AstSourceSpan location, AstNode* param, AstNode* body);
    AstNode* getParam();
    AstNode* getBody();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *param, *body;
};

class WhileStatement : public AstNode {
public:
    WhileStatement(AstSourceSpan location, AstNode* test, AstNode* body);
    AstNode* getTest();
    AstNode* getBody();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *test, *body;
};

class DoWhileStatement : public AstNode {
public:
    DoWhileStatement(AstSourceSpan location, AstNode* test, AstNode* body);
    AstNode* getTest();
    AstNode* getBody();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *test, *body;
};

class Function : public AstNode {
public:
    Identifier* getId();
    AstNode* getBody();
    TypeAnnotation* getReturnType();
    AstNode* getReturnTypeAnnotation();
    const std::vector<AstNode*>& getParams();
    bool isGenerator();
    bool isAsync();

protected:
    Function(AstSourceSpan location, AstNodeType type, AstNode* id, std::vector<AstNode*> params, AstNode* body,
             TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType, bool generator, bool async);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

protected:
    AstNode* id;
    std::vector<AstNode*> params;
    AstNode* body;
    TypeParameterDeclaration* typeParameters;
    TypeAnnotation* returnType;
    bool generator, async;
};

class ObjectProperty : public AstNode {
public:
    ObjectProperty(AstSourceSpan location, AstNode* key, AstNode* value, bool shorthand, bool computed);
    AstNode* getKey();
    Identifier* getValue();
    bool isShorthand();
    bool isComputed();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *key, *value;
    bool shorthand, computed;
};

class ObjectMethod : public Function {
public:
    enum class Kind {
        Constructor,
        Method,
        Get,
        Set,
    };

    ObjectMethod(AstSourceSpan location, AstNode* id, std::vector<AstNode*> params, AstNode* body, TypeParameterDeclaration* typeParameters,
                 TypeAnnotation* returnType, AstNode* key, Kind kind, bool isGenerator, bool isAsync, bool isComputed);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* key;
    Kind kind;
    bool isComputed;
};

class Super : public AstNode {
public:
    Super(AstSourceSpan location);
};

class Import : public AstNode {
public:
    Import(AstSourceSpan location);
};

class ThisExpression : public AstNode {
public:
    ThisExpression(AstSourceSpan location);
};

class ArrowFunctionExpression : public Function {
public:
    ArrowFunctionExpression(AstSourceSpan location, AstNode* id, std::vector<AstNode*> params, AstNode* body, TypeParameterDeclaration* typeParameters,
                            TypeAnnotation* returnType, bool isGenerator, bool isAsync, bool expression);

    bool isExpression();

private:
    bool expression;
};

class YieldExpression : public AstNode {
public:
    YieldExpression(AstSourceSpan location, AstNode* argument, bool isDelegate);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* argument;
    bool isDelegate;
};

class AwaitExpression : public AstNode {
public:
    AwaitExpression(AstSourceSpan location, AstNode* argument);
    AstNode* getArgument();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* argument;
};

class ArrayExpression : public AstNode {
public:
    ArrayExpression(AstSourceSpan location, std::vector<AstNode*> elements);
    const std::vector<AstNode*>& getElements();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    // NOTE: Some of the elements might be nullptrs! E.g. [1,,3] result in {NumericLiteral*,nullptr,NumericLiteral*}
    std::vector<AstNode*> elements;
};

class ObjectExpression : public AstNode {
public:
    ObjectExpression(AstSourceSpan location, std::vector<AstNode*> properties);
    const std::vector<AstNode*>& getProperties();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> properties;
};

class FunctionExpression : public Function {
public:
    FunctionExpression(AstSourceSpan location, AstNode* id, std::vector<AstNode*> params, AstNode* body,
                       TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType, bool isGenerator, bool isAsync);
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
    };
    UnaryExpression(AstSourceSpan location, AstNode* argument, Operator unaryOperator, bool isPrefix);
    AstNode* getArgument();
    Operator getOperator();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

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
    UpdateExpression(AstSourceSpan location, AstNode* argument, Operator updateOperator, bool prefix);
    AstNode* getArgument();
    Operator getOperator();
    bool isPrefix();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* argument;
    Operator updateOperator;
    bool prefix;
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
    BinaryExpression(AstSourceSpan location, AstNode* left, AstNode* right, Operator binaryOperator);
    AstNode* getLeft();
    AstNode* getRight();
    Operator getOperator();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

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
    AssignmentExpression(AstSourceSpan location, AstNode* left, AstNode* right, Operator assignmentOperator);
    AstNode* getLeft();
    AstNode* getRight();
    Operator getOperator();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

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
    LogicalExpression(AstSourceSpan location, AstNode* left, AstNode* right, Operator logicalOperator);
    AstNode* getLeft();
    AstNode* getRight();
    Operator getOperator();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *left, *right;
    Operator logicalOperator;
};

class MemberExpression : public AstNode {
public:
    MemberExpression(AstSourceSpan location, AstNode* object, AstNode* property, bool computed);
    AstNode* getObject();
    AstNode* getProperty();
    bool isComputed();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *object, *property;
    bool computed;
};

class BindExpression : public AstNode {
public:
    BindExpression(AstSourceSpan location, AstNode* object, AstNode* callee);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *object, *callee;
};

class ConditionalExpression : public AstNode {
public:
    ConditionalExpression(AstSourceSpan location, AstNode* test, AstNode* alternate, AstNode* consequent);
    AstNode* getTest();
    AstNode* getAlternate();
    AstNode* getConsequent();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *test, *alternate, *consequent;
};

class CallExpression : public AstNode {
public:
    CallExpression(AstSourceSpan location, AstNode* callee, std::vector<AstNode*> arguments);
    const std::vector<AstNode*>& getArguments();
    AstNode* getCallee();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

protected:
    CallExpression(AstSourceSpan location, AstNodeType type, AstNode* callee, std::vector<AstNode*> arguments);

private:
    AstNode* callee;
    std::vector<AstNode*> arguments;
};

class NewExpression : public CallExpression {
public:
    NewExpression(AstSourceSpan location, AstNode* callee, std::vector<AstNode*> arguments);
};

class SequenceExpression : public AstNode {
public:
    SequenceExpression(AstSourceSpan location, std::vector<AstNode*> expressions);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> expressions;
};

class DoExpression : public AstNode {
public:
    DoExpression(AstSourceSpan location, AstNode* body);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* body;
};

class Class : public AstNode {
public:
    Identifier* getId();
    ClassBody* getBody();
    TypeParameterDeclaration* getTypeParameters();
    const std::vector<ClassImplements*>& getImplements();

protected:
    Class(AstSourceSpan location, AstNodeType type, AstNode* id, AstNode* superClass, AstNode* body,
          TypeParameterDeclaration* typeParameters, TypeParameterInstantiation* superTypeParameters, std::vector<ClassImplements*> implements);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

protected:
    AstNode *id, *superClass;
    AstNode* body;
    TypeParameterDeclaration* typeParameters;
    TypeParameterInstantiation* superTypeParameters;
    std::vector<ClassImplements*> implements;
};

class ClassExpression : public Class {
public:
    ClassExpression(AstSourceSpan location, AstNode* id, AstNode* superClass, AstNode* body,
                    TypeParameterDeclaration* typeParameters, TypeParameterInstantiation* superTypeParameters, std::vector<ClassImplements*> implements);
};

class ClassDeclaration : public Class {
public:
    ClassDeclaration(AstSourceSpan location, AstNode* id, AstNode* superClass, AstNode* body,
                     TypeParameterDeclaration* typeParameters, TypeParameterInstantiation* superTypeParameters, std::vector<ClassImplements*> implements);
};

class ClassBody : public AstNode {
public:
    ClassBody(AstSourceSpan location, std::vector<AstNode*> body);
    const std::vector<AstNode*>& getBody();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> body;
};

class ClassProperty : public AstNode {
public:
    ClassProperty(AstSourceSpan location, AstNode* key, AstNode* value, TypeAnnotation* typeAnnotation, bool isStatic, bool isComputed);
    Identifier* getKey();
    AstNode* getValue();
    TypeAnnotation* getTypeAnnotation();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *key, *value;
    TypeAnnotation* typeAnnotation;
    bool isStatic, isComputed;
};

class ClassPrivateProperty : public AstNode {
public:
    ClassPrivateProperty(AstSourceSpan location, AstNode* key, AstNode* value, TypeAnnotation* typeAnnotation, bool isStatic);
    Identifier* getKey();
    AstNode* getValue();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *key, *value;
    TypeAnnotation* typeAnnotation;
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

    ClassMethod(AstSourceSpan location, AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key,
                TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType,
                Kind kind, bool isGenerator, bool isAsync, bool isComputed, bool isStatic);
    Identifier* getKey();
    Kind getKind();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

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

    ClassPrivateMethod(AstSourceSpan location, AstNode* id, std::vector<AstNode*> params, AstNode* body, AstNode* key,
                       TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType,
                       Kind kind, bool isGenerator, bool isAsync, bool isStatic);
    Identifier* getKey();
    Kind getKind();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* key;
    Kind kind;
    bool isStatic;
};

class FunctionDeclaration : public Function {
public:
    FunctionDeclaration(AstSourceSpan location, AstNode* id, std::vector<AstNode*> params, AstNode* body,
                        TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType, bool isGenerator, bool isAsync);
};

class VariableDeclaration : public AstNode {
public:
    enum class Kind {
        Var,
        Let,
        Const
    };
    VariableDeclaration(AstSourceSpan location, std::vector<VariableDeclarator*> declarators, Kind kind);
    Kind getKind();
    const std::vector<VariableDeclarator*>& getDeclarators();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<VariableDeclarator*> declarators;
    Kind kind;
};

class VariableDeclarator : public AstNode {
public:
    VariableDeclarator(AstSourceSpan location, AstNode* id, AstNode* init);
    AstNode *getId();
    AstNode* getInit();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *id, *init;
};

class ForStatement : public AstNode {
public:
    ForStatement(AstSourceSpan location, AstNode* init, AstNode* test, AstNode* update, AstNode* body);
    VariableDeclaration* getInit();
    AstNode* getTest();
    AstNode* getUpdate();
    AstNode* getBody();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *init, *test, *update, *body;
};

class ForInStatement : public AstNode {
public:
    ForInStatement(AstSourceSpan location, AstNode* left, AstNode* right, AstNode* body);
    AstNode* getLeft();
    AstNode* getRight();
    AstNode* getBody();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *left, *right, *body;
};

class ForOfStatement : public AstNode {
public:
    ForOfStatement(AstSourceSpan location, AstNode* left, AstNode* right, AstNode* body, bool isAwait);
    AstNode* getLeft();
    AstNode* getRight();
    AstNode* getBody();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *left, *right, *body;
    bool isAwait;
};

class SpreadElement : public AstNode {
public:
    SpreadElement(AstSourceSpan location, AstNode* argument);
    AstNode* getArgument();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* argument;
};

class ObjectPattern : public AstNode {
public:
    ObjectPattern(AstSourceSpan location, std::vector<AstNode*> properties, TypeAnnotation* typeAnnotation);
    const std::vector<AstNode*>& getProperties();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> properties;
    TypeAnnotation* typeAnnotation;
};

class ArrayPattern : public AstNode {
public:
    ArrayPattern(AstSourceSpan location, std::vector<AstNode*> elements);
    const std::vector<AstNode*>& getElements();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> elements;
};

// Typically appears in the parameter for a function
class AssignmentPattern : public AstNode {
public:
    AssignmentPattern(AstSourceSpan location, AstNode* left, AstNode* right);
    AstNode* getLeft(); // Identifier, ObjectPattern or ArrayPattern
    AstNode* getRight();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *left, *right;
};

class RestElement : public AstNode {
public:
    RestElement(AstSourceSpan location, AstNode* argument, TypeAnnotation* typeAnnotation);
    Identifier* getArgument();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* argument;
    TypeAnnotation* typeAnnotation;
};

class MetaProperty : public AstNode {
public:
    MetaProperty(AstSourceSpan location, AstNode* meta, AstNode* property);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *meta, *property;
};

class ImportDeclaration : public AstNode {
public:
    enum class Kind {
        Value,
        Type,
        None,
    };

public:
    ImportDeclaration(AstSourceSpan location, std::vector<AstNode*> specifiers, AstNode* source, Kind kind);
    std::string getSource();
    Kind getKind();
    const std::vector<AstNode*>& getSpecifiers();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> specifiers;
    AstNode* source;
    Kind kind;
};

class ImportSpecifier : public AstNode {
public:
    ImportSpecifier(AstSourceSpan location, AstNode* local, AstNode* imported, bool typeImport);
    Identifier* getLocal();
    Identifier* getImported();
    bool isTypeImport();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier *local, *imported;
    bool localEqualsImported;
    bool typeImport;
};

class ImportDefaultSpecifier : public AstNode {
public:
    ImportDefaultSpecifier(AstSourceSpan location, AstNode* local);
    Identifier* getLocal();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* local;
};

class ImportNamespaceSpecifier : public AstNode {
public:
    ImportNamespaceSpecifier(AstSourceSpan location, AstNode* local);
    Identifier* getLocal();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* local;
};

class ExportNamedDeclaration : public AstNode {
public:
    enum class Kind {
        Value,
        Type,
    };

public:
    ExportNamedDeclaration(AstSourceSpan location, AstNode* declaration, AstNode* source, std::vector<AstNode*> specifiers, Kind kind);
    Kind getKind();
    AstNode* getSource();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *declaration, *source;
    std::vector<AstNode*> specifiers;
    Kind kind;
};

class ExportDefaultDeclaration : public AstNode {
public:
    ExportDefaultDeclaration(AstSourceSpan location, AstNode* declaration);
    AstNode* getDeclaration();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* declaration;
};

class ExportAllDeclaration : public AstNode {
public:
    ExportAllDeclaration(AstSourceSpan location, AstNode* source);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* source;
};

class ExportSpecifier : public AstNode {
public:
    ExportSpecifier(AstSourceSpan location, AstNode* local, AstNode* exported);
    Identifier* getLocal();
    Identifier* getExported();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *local, *exported;
};

class ExportDefaultSpecifier : public AstNode {
public:
    ExportDefaultSpecifier(AstSourceSpan location, AstNode* exported);
    Identifier* getExported();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *exported;
};

class TypeAnnotation : public AstNode {
public:
    TypeAnnotation(AstSourceSpan location, AstNode* typeAnnotation);
    AstNode* getTypeAnnotation();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *typeAnnotation;
};

class GenericTypeAnnotation : public AstNode {
public:
    GenericTypeAnnotation(AstSourceSpan location, AstNode* id, AstNode* typeParameters);
    Identifier* getId();
    Identifier* getTypeParameters();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *id;
    AstNode *typeParameters;
};

class StringTypeAnnotation : public AstNode {
public:
    StringTypeAnnotation(AstSourceSpan location);
};

class NumberTypeAnnotation : public AstNode {
public:
    NumberTypeAnnotation(AstSourceSpan location);
};

class BooleanTypeAnnotation : public AstNode {
public:
    BooleanTypeAnnotation(AstSourceSpan location);
};

class VoidTypeAnnotation : public AstNode {
public:
    VoidTypeAnnotation(AstSourceSpan location);
};

class AnyTypeAnnotation : public AstNode {
public:
    AnyTypeAnnotation(AstSourceSpan location);
};

class ExistsTypeAnnotation : public AstNode {
public:
    ExistsTypeAnnotation(AstSourceSpan location);
};

class MixedTypeAnnotation : public AstNode {
public:
    MixedTypeAnnotation(AstSourceSpan location);
};

class NullableTypeAnnotation : public AstNode {
public:
    NullableTypeAnnotation(AstSourceSpan location, AstNode* typeAnnotation);
    AstNode* getTypeAnnotation();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* typeAnnotation;
};

class ArrayTypeAnnotation : public AstNode {
public:
    ArrayTypeAnnotation(AstSourceSpan location, AstNode* elementType);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* elementType;
};

class TupleTypeAnnotation : public AstNode {
public:
    TupleTypeAnnotation(AstSourceSpan location, std::vector<AstNode*> types);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> types;
};

class UnionTypeAnnotation : public AstNode {
public:
    UnionTypeAnnotation(AstSourceSpan location, std::vector<AstNode*> types);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> types;
};

class NullLiteralTypeAnnotation : public AstNode {
public:
    NullLiteralTypeAnnotation(AstSourceSpan location);
};

class NumberLiteralTypeAnnotation : public AstNode {
public:
    NumberLiteralTypeAnnotation(AstSourceSpan location, double value);
    double getValue();

private:
    double value;
};

class StringLiteralTypeAnnotation : public AstNode {
public:
    StringLiteralTypeAnnotation(AstSourceSpan location, std::string value);

private:
    std::string value;
};

class BooleanLiteralTypeAnnotation : public AstNode {
public:
    BooleanLiteralTypeAnnotation(AstSourceSpan location, bool value);
    bool getValue();

private:
    bool value;
};

class TypeofTypeAnnotation : public AstNode {
public:
    TypeofTypeAnnotation(AstSourceSpan location, AstNode* argument);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* argument;
};

class FunctionTypeAnnotation : public AstNode {
public:
    FunctionTypeAnnotation(AstSourceSpan location, std::vector<FunctionTypeParam*> params, FunctionTypeParam* rest, AstNode* returnType);
    const std::vector<FunctionTypeParam*>& getParams();
    FunctionTypeParam* getRestParam();
    AstNode* getReturnType();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<FunctionTypeParam*> params;
    FunctionTypeParam* rest;
    AstNode* returnType;
};

class FunctionTypeParam : public AstNode {
public:
    FunctionTypeParam(AstSourceSpan location, Identifier* name, AstNode* typeAnnotation);
    Identifier* getName();
    AstNode* getTypeAnnotation();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier* name;
    AstNode* typeAnnotation;
};

class ObjectTypeAnnotation : public AstNode {
public:
    ObjectTypeAnnotation(AstSourceSpan location, std::vector<AstNode*> properties, std::vector<ObjectTypeIndexer*> indexers, bool exact);
    const std::vector<AstNode*>& getProperties();
    bool isExact();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> properties;
    std::vector<ObjectTypeIndexer*> indexers;
    bool exact;
};

class ObjectTypeProperty : public AstNode {
public:
    ObjectTypeProperty(AstSourceSpan location, Identifier* key, AstNode* value, bool optional);
    Identifier* getKey();
    AstNode* getValue();
    bool isOptional();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier *key;
    AstNode *value;
    bool optional;
};

class ObjectTypeSpreadProperty : public AstNode {
public:
    ObjectTypeSpreadProperty(AstSourceSpan location, AstNode* argument);
    AstNode* getArgument();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode *argument;
};

class ObjectTypeIndexer : public AstNode {
public:
    ObjectTypeIndexer(AstSourceSpan location, Identifier* id, AstNode* key, AstNode* value);
    Identifier* getId();
    AstNode* getKey();
    AstNode* getValue();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier *id;
    AstNode *key;
    AstNode *value;
};

class TypeAlias : public AstNode {
public:
    TypeAlias(AstSourceSpan location, Identifier* id, AstNode* typeParameters, AstNode* right);
    Identifier* getId();
    AstNode* getTypeParameters();
    AstNode* getRight();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier *id;
    AstNode *typeParameters;
    AstNode *right;
};

class TypeParameterInstantiation : public AstNode {
public:
    TypeParameterInstantiation(AstSourceSpan location, std::vector<AstNode*> params);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<AstNode*> params;
};

class TypeParameterDeclaration : public AstNode {
public:
    TypeParameterDeclaration(AstSourceSpan location, std::vector<TypeParameter*> params);
    const std::vector<TypeParameter*>& getParams();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    std::vector<TypeParameter*> params;
};

class TypeParameter : public AstNode {
public:
    TypeParameter(AstSourceSpan location, std::string name, AstNode* bound);
    Identifier* getName();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier* name; // We pretend our name is an identifier for consistency
    AstNode* bound;
};

class TypeCastExpression : public AstNode {
public:
    TypeCastExpression(AstSourceSpan location, AstNode* expression, TypeAnnotation* typeAnnotation);
    AstNode* getExpression();
    TypeAnnotation* getTypeAnnotation();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* expression;
    TypeAnnotation* typeAnnotation;
};

class ClassImplements : public AstNode {
public:
    ClassImplements(AstSourceSpan location, Identifier* id, TypeParameterInstantiation* typeParameters);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier* id;
    TypeParameterInstantiation* typeParameters;
};

class QualifiedTypeIdentifier : public AstNode {
public:
    QualifiedTypeIdentifier(AstSourceSpan location, Identifier* qualification, Identifier* id);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;
    Identifier *getId();
    Identifier *getQualification();

private:
    Identifier* qualification;
    Identifier* id;
};

class InterfaceDeclaration : public AstNode {
public:
    InterfaceDeclaration(AstSourceSpan location, Identifier* id, TypeParameterDeclaration* typeParameters, AstNode* body,
                         std::vector<InterfaceExtends*> extends, std::vector<InterfaceExtends*> mixins);
    Identifier* getId();
    TypeParameterDeclaration* getTypeParameters();
    AstNode* getBody();
    const std::vector<InterfaceExtends*>& getExtends();
    const std::vector<InterfaceExtends*>& getMixins();
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier* id;
    TypeParameterDeclaration* typeParameters;
    AstNode* body;
    std::vector<InterfaceExtends*> extends;
    std::vector<InterfaceExtends*> mixins;
};

class InterfaceExtends : public AstNode {
public:
    InterfaceExtends(AstSourceSpan location, Identifier* id, TypeParameterInstantiation* typeParameters);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier* id;
    TypeParameterInstantiation* typeParameters;
};

class DeclareVariable : public AstNode {
public:
    DeclareVariable(AstSourceSpan location, Identifier* id);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier* id;
};

class DeclareFunction : public AstNode {
public:
    DeclareFunction(AstSourceSpan location, Identifier* id);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier* id;
};

class DeclareTypeAlias : public AstNode {
public:
    DeclareTypeAlias(AstSourceSpan location, Identifier* id, AstNode* right);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier* id;
    AstNode* right;
};

class DeclareClass : public AstNode {
public:
    DeclareClass(AstSourceSpan location, Identifier* id, TypeParameterDeclaration* typeParameters, AstNode* body,
                 std::vector<InterfaceExtends*> extends, std::vector<InterfaceExtends*> mixins);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    Identifier* id;
    TypeParameterDeclaration* typeParameters;
    AstNode* body;
    std::vector<InterfaceExtends*> extends;
    std::vector<InterfaceExtends*> mixins;
};

class DeclareModule : public AstNode {
public:
    DeclareModule(AstSourceSpan location, StringLiteral* id, AstNode* body);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    StringLiteral* id;
    AstNode* body;
};

class DeclareExportDeclaration : public AstNode {
public:
    DeclareExportDeclaration(AstSourceSpan location, AstNode* declaration);
    virtual void applyChildren(const std::function<bool (AstNode*)>&) override;

private:
    AstNode* declaration;
};

#endif // AST_HPP
