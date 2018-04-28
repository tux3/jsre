#ifndef IMPORT_HPP
#define IMPORT_HPP

#include <json.hpp>

#define IMPORTED_NODE_LIST(X)   \
    X(Identifier)               \
    X(RegExpLiteral)            \
    X(NullLiteral)              \
    X(StringLiteral)            \
    X(BooleanLiteral)           \
    X(NumericLiteral)           \
    X(TemplateLiteral)          \
    X(TemplateElement)          \
    X(TaggedTemplateExpression) \
    X(ObjectProperty)           \
    X(ObjectMethod)             \
    X(ExpressionStatement)      \
    X(BlockStatement)           \
    X(EmptyStatement)           \
    X(WithStatement)            \
    X(DebuggerStatement)        \
    X(ReturnStatement)          \
    X(LabeledStatement)         \
    X(BreakStatement)           \
    X(ContinueStatement)        \
    X(IfStatement)              \
    X(SwitchStatement)          \
    X(SwitchCase)               \
    X(ThrowStatement)           \
    X(TryStatement)             \
    X(CatchClause)              \
    X(WhileStatement)           \
    X(DoWhileStatement)         \
    X(ForStatement)             \
    X(ForInStatement)           \
    X(ForOfStatement)           \
    X(Super)                    \
    X(Import)                   \
    X(ThisExpression)           \
    X(ArrowFunctionExpression)  \
    X(YieldExpression)          \
    X(AwaitExpression)          \
    X(ArrayExpression)          \
    X(ObjectExpression)         \
    X(ConditionalExpression)    \
    X(FunctionExpression)       \
    X(UnaryExpression)          \
    X(UpdateExpression)         \
    X(BinaryExpression)         \
    X(AssignmentExpression)     \
    X(LogicalExpression)        \
    X(MemberExpression)         \
    X(BindExpression)           \
    X(CallExpression)           \
    X(NewExpression)            \
    X(SequenceExpression)       \
    X(DoExpression)             \
    X(ClassExpression)          \
    X(ClassBody)                \
    X(ClassMethod)              \
    X(ClassPrivateMethod)       \
    X(ClassProperty)            \
    X(ClassPrivateProperty)     \
    X(ClassDeclaration)         \
    X(VariableDeclaration)      \
    X(FunctionDeclaration)      \
    X(VariableDeclarator)       \
    X(SpreadElement)            \
    X(ObjectPattern)            \
    X(ArrayPattern)             \
    X(AssignmentPattern)        \
    X(RestElement)              \
    X(MetaProperty)             \
    X(ImportDeclaration)        \
    X(ImportSpecifier)          \
    X(ImportDefaultSpecifier)   \
    X(ImportNamespaceSpecifier) \
    X(ExportNamedDeclaration)   \
    X(ExportDefaultDeclaration) \
    X(ExportAllDeclaration)     \
    X(ExportSpecifier)          \
    X(ExportDefaultSpecifier)

class AstRoot;
class AstNode;
class Module;

AstRoot* importBabylonAst(Module &parentModule, const nlohmann::json& ast);
AstNode* importNode(const nlohmann::json& node);
AstNode* importNodeOrNullptr(const nlohmann::json& node, const char* name);

#endif // IMPORT_HPP
