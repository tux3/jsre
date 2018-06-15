#ifndef AST_HPP
#define AST_HPP

#include "import.hpp"
#include "location.hpp"
#include <string>
#include <vector>

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
    Module& getParentModule();
    AstNodeType getType();
    const char* getTypeName();
    AstSourceSpan getLocation(); //< Location of the AST node. Don't forget this is UTF8, not bytes!
    virtual std::vector<AstNode*> getChildren();

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
    Module& getParentModule();
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::string name;
    TypeAnnotation* typeAnnotation;
    bool optional; // Flow syntax, e.g. for optional parameters
};

class RegExpLiteral : public AstNode {
public:
    RegExpLiteral(AstSourceSpan location, std::string pattern, std::string flags);

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
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *tag, *quasi;
};


class ExpressionStatement : public AstNode {
public:
    ExpressionStatement(AstSourceSpan location, AstNode* expression);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* expression;
};

class BlockStatement : public AstNode {
public:
    BlockStatement(AstSourceSpan location, std::vector<AstNode*> body);
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* argument;
};

class LabeledStatement : public AstNode {
public:
    LabeledStatement(AstSourceSpan location, AstNode* label, AstNode* body);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *label, *body;
};

class BreakStatement : public AstNode {
public:
    BreakStatement(AstSourceSpan location, AstNode* label);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* label;
};

class ContinueStatement : public AstNode {
public:
    ContinueStatement(AstSourceSpan location, AstNode* label);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* label;
};

class IfStatement : public AstNode {
public:
    IfStatement(AstSourceSpan location, AstNode* test, AstNode* consequent, AstNode* argument);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *test, *consequent, *argument;
};

class SwitchStatement : public AstNode {
public:
    SwitchStatement(AstSourceSpan location, AstNode* discriminant, std::vector<AstNode*> cases);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* discriminant;
    std::vector<AstNode*> cases;
};

class SwitchCase : public AstNode {
public:
    SwitchCase(AstSourceSpan location, AstNode* testOrDefault, std::vector<AstNode*> consequent);
    const std::vector<AstNode*>& getConsequent();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* testOrDefault;
    std::vector<AstNode*> consequent;
};

class ThrowStatement : public AstNode {
public:
    ThrowStatement(AstSourceSpan location, AstNode* argument);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* argument;
};

class TryStatement : public AstNode {
public:
    TryStatement(AstSourceSpan location, AstNode* body, AstNode* handler, AstNode* finalizer);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *body, *handler, *finalizer;
};

class CatchClause : public AstNode {
public:
    CatchClause(AstSourceSpan location, AstNode* param, AstNode* body);
    AstNode* getParam();
    AstNode* getBody();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *param, *body;
};

class WhileStatement : public AstNode {
public:
    WhileStatement(AstSourceSpan location, AstNode* test, AstNode* body);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *test, *body;
};

class DoWhileStatement : public AstNode {
public:
    DoWhileStatement(AstSourceSpan location, AstNode* test, AstNode* body);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *test, *body;
};

class Function : public AstNode {
public:
    Identifier* getId();
    BlockStatement* getBody();
    const std::vector<Identifier*>& getParams();

protected:
    Function(AstSourceSpan location, AstNodeType type, AstNode* id, std::vector<AstNode*> params, AstNode* body,
             TypeParameterDeclaration* typeParameters, TypeAnnotation* returnType, bool generator, bool async);
    bool isGenerator();
    bool isAsync();
    virtual std::vector<AstNode*> getChildren() override;

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
    ObjectProperty(AstSourceSpan location, AstNode* key, AstNode* value, bool isShorthand, bool isComputed);
    Identifier* getKey();
    virtual std::vector<AstNode*> getChildren() override;

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

    ObjectMethod(AstSourceSpan location, AstNode* id, std::vector<AstNode*> params, AstNode* body, TypeParameterDeclaration* typeParameters,
                 TypeAnnotation* returnType, AstNode* key, Kind kind, bool isGenerator, bool isAsync, bool isComputed);
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* argument;
    bool isDelegate;
};

class AwaitExpression : public AstNode {
public:
    AwaitExpression(AstSourceSpan location, AstNode* argument);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* argument;
};

class ArrayExpression : public AstNode {
public:
    ArrayExpression(AstSourceSpan location, std::vector<AstNode*> elements);
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::vector<AstNode*> elements;
};

class ObjectExpression : public AstNode {
public:
    ObjectExpression(AstSourceSpan location, std::vector<AstNode*> properties);
    virtual std::vector<AstNode*> getChildren() override;

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
        Throw,
    };
    UnaryExpression(AstSourceSpan location, AstNode* argument, Operator unaryOperator, bool isPrefix);
    virtual std::vector<AstNode*> getChildren() override;

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
    UpdateExpression(AstSourceSpan location, AstNode* argument, Operator updateOperator, bool isPrefix);
    virtual std::vector<AstNode*> getChildren() override;

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
    BinaryExpression(AstSourceSpan location, AstNode* left, AstNode* right, Operator binaryOperator);
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *left, *right;
    Operator logicalOperator;
};

class MemberExpression : public AstNode {
public:
    MemberExpression(AstSourceSpan location, AstNode* object, AstNode* property, bool isComputed);
    Identifier* getProperty();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *object, *property;
    bool isComputed;
};

class BindExpression : public AstNode {
public:
    BindExpression(AstSourceSpan location, AstNode* object, AstNode* callee);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *object, *callee;
};

class ConditionalExpression : public AstNode {
public:
    ConditionalExpression(AstSourceSpan location, AstNode* test, AstNode* alternate, AstNode* consequent);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *test, *alternate, *consequent;
};

class CallExpression : public AstNode {
public:
    CallExpression(AstSourceSpan location, AstNode* callee, std::vector<AstNode*> arguments);
    const std::vector<AstNode*>& getArguments();
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::vector<AstNode*> expressions;
};

class DoExpression : public AstNode {
public:
    DoExpression(AstSourceSpan location, AstNode* body);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* body;
};

class Class : public AstNode {
public:
    Identifier* getId();
    TypeParameterDeclaration* getTypeParameters();
    const std::vector<ClassImplements*>& getImplements();

protected:
    Class(AstSourceSpan location, AstNodeType type, AstNode* id, AstNode* superClass, AstNode* body,
          TypeParameterDeclaration* typeParameters, TypeParameterInstantiation* superTypeParameters, std::vector<ClassImplements*> implements);
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::vector<AstNode*> body;
};

class ClassProperty : public AstNode {
public:
    ClassProperty(AstSourceSpan location, AstNode* key, AstNode* value, TypeAnnotation* typeAnnotation, bool isStatic, bool isComputed);
    Identifier* getKey();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *key, *value;
    TypeAnnotation* typeAnnotation;
    bool isStatic, isComputed;
};

class ClassPrivateProperty : public AstNode {
public:
    ClassPrivateProperty(AstSourceSpan location, AstNode* key, AstNode* value, TypeAnnotation* typeAnnotation, bool isStatic);
    Identifier* getKey();
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

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
    VariableDeclaration(AstSourceSpan location, std::vector<AstNode*> declarators, Kind kind);
    Kind getKind();
    const std::vector<AstNode*>& getDeclarators();
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::vector<AstNode*> declarators;
    Kind kind;
};

class VariableDeclarator : public AstNode {
public:
    VariableDeclarator(AstSourceSpan location, AstNode* id, AstNode* init);
    AstNode* getId();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *id, *init;
};

class ForStatement : public AstNode {
public:
    ForStatement(AstSourceSpan location, AstNode* init, AstNode* test, AstNode* update, AstNode* body);
    VariableDeclaration* getInit();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *init, *test, *update, *body;
};

class ForInStatement : public AstNode {
public:
    ForInStatement(AstSourceSpan location, AstNode* left, AstNode* right, AstNode* body);
    AstNode* getLeft();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *left, *right, *body;
};

class ForOfStatement : public AstNode {
public:
    ForOfStatement(AstSourceSpan location, AstNode* left, AstNode* right, AstNode* body, bool isAwait);
    AstNode* getLeft();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *left, *right, *body;
    bool isAwait;
};

class SpreadElement : public AstNode {
public:
    SpreadElement(AstSourceSpan location, AstNode* argument);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* argument;
};

class ObjectPattern : public AstNode {
public:
    ObjectPattern(AstSourceSpan location, std::vector<AstNode*> properties, TypeAnnotation* typeAnnotation);
    const std::vector<ObjectProperty*>& getProperties();
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::vector<AstNode*> properties;
    TypeAnnotation* typeAnnotation;
};

class ArrayPattern : public AstNode {
public:
    ArrayPattern(AstSourceSpan location, std::vector<AstNode*> elements);
    const std::vector<AstNode*>& getElements();
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::vector<AstNode*> elements;
};

class AssignmentPattern : public AstNode {
public:
    AssignmentPattern(AstSourceSpan location, AstNode* left, AstNode* right);
    Identifier* getLeft();
    AstNode* getRight();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *left, *right;
};

class RestElement : public AstNode {
public:
    RestElement(AstSourceSpan location, AstNode* argument, TypeAnnotation* typeAnnotation);
    Identifier* getArgument();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* argument;
    TypeAnnotation* typeAnnotation;
};

class MetaProperty : public AstNode {
public:
    MetaProperty(AstSourceSpan location, AstNode* meta, AstNode* property);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *meta, *property;
};

class ImportDeclaration : public AstNode {
public:
    enum class Kind {
        Value,
        Type,
    };

public:
    ImportDeclaration(AstSourceSpan location, std::vector<AstNode*> specifiers, AstNode* source, Kind kind);
    std::string getSource();
    Kind getKind();
    const std::vector<AstNode*>& getSpecifiers();
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    Identifier *local, *imported;
    bool localEqualsImported;
    bool typeImport;
};

class ImportDefaultSpecifier : public AstNode {
public:
    ImportDefaultSpecifier(AstSourceSpan location, AstNode* local);
    Identifier* getLocal();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* local;
};

class ImportNamespaceSpecifier : public AstNode {
public:
    ImportNamespaceSpecifier(AstSourceSpan location, AstNode* local);
    Identifier* getLocal();
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *declaration, *source;
    std::vector<AstNode*> specifiers;
    Kind kind;
};

class ExportDefaultDeclaration : public AstNode {
public:
    ExportDefaultDeclaration(AstSourceSpan location, AstNode* declaration);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* declaration;
};

class ExportAllDeclaration : public AstNode {
public:
    ExportAllDeclaration(AstSourceSpan location, AstNode* source);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* source;
};

class ExportSpecifier : public AstNode {
public:
    ExportSpecifier(AstSourceSpan location, AstNode* local, AstNode* exported);
    Identifier* getLocal();
    Identifier* getExported();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *local, *exported;
};

class ExportDefaultSpecifier : public AstNode {
public:
    ExportDefaultSpecifier(AstSourceSpan location, AstNode* exported);
    Identifier* getExported();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *exported;
};

class TypeAnnotation : public AstNode {
public:
    TypeAnnotation(AstSourceSpan location, AstNode* typeAnnotation);
    Identifier* getTypeAnnotation();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *typeAnnotation;
};

class GenericTypeAnnotation : public AstNode {
public:
    GenericTypeAnnotation(AstSourceSpan location, AstNode* id, AstNode* typeParameters);
    Identifier* getId();
    Identifier* getTypeParameters();
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* typeAnnotation;
};

class ArrayTypeAnnotation : public AstNode {
public:
    ArrayTypeAnnotation(AstSourceSpan location, AstNode* elementType);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* elementType;
};

class TupleTypeAnnotation : public AstNode {
public:
    TupleTypeAnnotation(AstSourceSpan location, std::vector<AstNode*> types);
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::vector<AstNode*> types;
};

class UnionTypeAnnotation : public AstNode {
public:
    UnionTypeAnnotation(AstSourceSpan location, std::vector<AstNode*> types);
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* argument;
};

class FunctionTypeAnnotation : public AstNode {
public:
    FunctionTypeAnnotation(AstSourceSpan location, std::vector<FunctionTypeParam*> params, FunctionTypeParam* rest, AstNode* returnType);
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::vector<FunctionTypeParam*> params;
    FunctionTypeParam* rest;
    AstNode* returnType;
};

class FunctionTypeParam : public AstNode {
public:
    FunctionTypeParam(AstSourceSpan location, Identifier* name, AstNode* typeAnnotation);
    Identifier* getName();
    virtual std::vector<AstNode*> getChildren() override;

private:
    Identifier* name;
    AstNode* typeAnnotation;
};

class ObjectTypeAnnotation : public AstNode {
public:
    ObjectTypeAnnotation(AstSourceSpan location, std::vector<ObjectTypeProperty*> properties, std::vector<ObjectTypeIndexer*> indexers, bool exact);
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::vector<ObjectTypeProperty*> properties;
    std::vector<ObjectTypeIndexer*> indexers;
    bool exact;
};

class ObjectTypeProperty : public AstNode {
public:
    ObjectTypeProperty(AstSourceSpan location, Identifier* key, AstNode* value, bool optional);
    Identifier* getKey();
    AstNode* getValue();
    bool isOptional();
    virtual std::vector<AstNode*> getChildren() override;

private:
    Identifier *key;
    AstNode *value;
    bool optional;
};

class ObjectTypeSpreadProperty : public AstNode {
public:
    ObjectTypeSpreadProperty(AstSourceSpan location, AstNode* argument);
    AstNode* getArgument();
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode *argument;
};

class ObjectTypeIndexer : public AstNode {
public:
    ObjectTypeIndexer(AstSourceSpan location, Identifier* id, AstNode* key, AstNode* value);
    Identifier* getId();
    AstNode* getKey();
    AstNode* getValue();
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    Identifier *id;
    AstNode *typeParameters;
    AstNode *right;
};

class TypeParameterInstantiation : public AstNode {
public:
    TypeParameterInstantiation(AstSourceSpan location, std::vector<AstNode*> params);
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::vector<AstNode*> params;
};

class TypeParameterDeclaration : public AstNode {
public:
    TypeParameterDeclaration(AstSourceSpan location, std::vector<AstNode*> params);
    const std::vector<TypeParameter*>& getParams();
    virtual std::vector<AstNode*> getChildren() override;

private:
    std::vector<AstNode*> params;
};

class TypeParameter : public AstNode {
public:
    TypeParameter(AstSourceSpan location, std::string name);
    Identifier* getName();
    virtual std::vector<AstNode*> getChildren() override;

private:
    Identifier* name; // We pretend our name is an identifier for consistency
};

class TypeCastExpression : public AstNode {
public:
    TypeCastExpression(AstSourceSpan location, AstNode* expression, TypeAnnotation* typeAnnotation);
    virtual std::vector<AstNode*> getChildren() override;

private:
    AstNode* expression;
    TypeAnnotation* typeAnnotation;
};

class ClassImplements : public AstNode {
public:
    ClassImplements(AstSourceSpan location, Identifier* id, TypeParameterInstantiation* typeParameters);
    virtual std::vector<AstNode*> getChildren() override;

private:
    Identifier* id;
    TypeParameterInstantiation* typeParameters;
};

class QualifiedTypeIdentifier : public AstNode {
public:
    QualifiedTypeIdentifier(AstSourceSpan location, Identifier* qualification, Identifier* id);
    virtual std::vector<AstNode*> getChildren() override;
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
    virtual std::vector<AstNode*> getChildren() override;

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
    virtual std::vector<AstNode*> getChildren() override;

private:
    Identifier* id;
    TypeParameterInstantiation* typeParameters;
};

#endif // AST_HPP
