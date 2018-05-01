#ifndef IMPORT_HPP
#define IMPORT_HPP

#include "ast/location.hpp"
#include <v8.h>

#define IMPORTED_NODE_LIST(X)       \
    X(Identifier)                   \
    X(RegExpLiteral)                \
    X(NullLiteral)                  \
    X(StringLiteral)                \
    X(BooleanLiteral)               \
    X(NumericLiteral)               \
    X(TemplateLiteral)              \
    X(TemplateElement)              \
    X(TaggedTemplateExpression)     \
    X(ObjectProperty)               \
    X(ObjectMethod)                 \
    X(ExpressionStatement)          \
    X(BlockStatement)               \
    X(EmptyStatement)               \
    X(WithStatement)                \
    X(DebuggerStatement)            \
    X(ReturnStatement)              \
    X(LabeledStatement)             \
    X(BreakStatement)               \
    X(ContinueStatement)            \
    X(IfStatement)                  \
    X(SwitchStatement)              \
    X(SwitchCase)                   \
    X(ThrowStatement)               \
    X(TryStatement)                 \
    X(CatchClause)                  \
    X(WhileStatement)               \
    X(DoWhileStatement)             \
    X(ForStatement)                 \
    X(ForInStatement)               \
    X(ForOfStatement)               \
    X(Super)                        \
    X(Import)                       \
    X(ThisExpression)               \
    X(ArrowFunctionExpression)      \
    X(YieldExpression)              \
    X(AwaitExpression)              \
    X(ArrayExpression)              \
    X(ObjectExpression)             \
    X(ConditionalExpression)        \
    X(FunctionExpression)           \
    X(UnaryExpression)              \
    X(UpdateExpression)             \
    X(BinaryExpression)             \
    X(AssignmentExpression)         \
    X(LogicalExpression)            \
    X(MemberExpression)             \
    X(BindExpression)               \
    X(CallExpression)               \
    X(NewExpression)                \
    X(SequenceExpression)           \
    X(DoExpression)                 \
    X(ClassExpression)              \
    X(ClassBody)                    \
    X(ClassMethod)                  \
    X(ClassPrivateMethod)           \
    X(ClassProperty)                \
    X(ClassPrivateProperty)         \
    X(ClassDeclaration)             \
    X(VariableDeclaration)          \
    X(FunctionDeclaration)          \
    X(VariableDeclarator)           \
    X(SpreadElement)                \
    X(ObjectPattern)                \
    X(ArrayPattern)                 \
    X(AssignmentPattern)            \
    X(RestElement)                  \
    X(MetaProperty)                 \
    X(ImportDeclaration)            \
    X(ImportSpecifier)              \
    X(ImportDefaultSpecifier)       \
    X(ImportNamespaceSpecifier)     \
    X(ExportNamedDeclaration)       \
    X(ExportDefaultDeclaration)     \
    X(ExportAllDeclaration)         \
    X(ExportSpecifier)              \
    X(ExportDefaultSpecifier)       \
    X(TypeAnnotation)               \
    X(GenericTypeAnnotation)        \
    X(TypeParameterInstantiation)   \
    X(TypeParameterDeclaration)     \
    X(TypeParameter)                \
    X(StringTypeAnnotation)         \
    X(NumberTypeAnnotation)         \
    X(BooleanTypeAnnotation)        \
    X(VoidTypeAnnotation)           \
    X(AnyTypeAnnotation)            \
    X(ExistsTypeAnnotation)         \
    X(MixedTypeAnnotation)          \
    X(NullableTypeAnnotation)       \
    X(TupleTypeAnnotation)          \
    X(UnionTypeAnnotation)          \
    X(TypeofTypeAnnotation)         \
    X(NullLiteralTypeAnnotation)    \
    X(NumberLiteralTypeAnnotation)  \
    X(StringLiteralTypeAnnotation)  \
    X(BooleanLiteralTypeAnnotation) \
    X(FunctionTypeAnnotation)       \
    X(FunctionTypeParam)            \
    X(ObjectTypeAnnotation)         \
    X(ObjectTypeProperty)           \
    X(ObjectTypeIndexer)            \
    X(TypeAlias)                    \
    X(TypeCastExpression)           \
    X(ClassImplements)              \
    X(QualifiedTypeIdentifier)      \
    X(InterfaceDeclaration)         \
    X(InterfaceExtends)

class AstRoot;
class AstNode;
class Module;

AstRoot* importBabylonAst(Module& parentModule, v8::Local<v8::Object> astObj);
AstNode* importNode(const v8::Local<v8::Object>& node);
AstNode* importChildOrNullptr(const v8::Local<v8::Object>& node, const char* name);
AstSourceSpan importLocation(const v8::Local<v8::Object>& node);

#endif // IMPORT_HPP
