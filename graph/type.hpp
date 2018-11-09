#ifndef TYPE_H
#define TYPE_H

#include <cstdint>

#define GRAPH_NODE_TYPE_LIST(X) \
    X(Start)                    \
    X(End)                      \
    X(Return)                   \
    X(Try)                      \
    X(PrepareException)         \
    X(Throw)                    \
    X(CatchException)           \
    X(Undefined)                \
    X(Literal)                  \
    X(ObjectLiteral)            \
    X(ObjectProperty)           \
    X(ArrayLiteral)             \
    X(TemplateLiteral)          \
    X(This)                     \
    X(Super)                    \
    X(Call)                     \
    X(NewCall)                  \
    X(Await)                    \
    X(BinaryOperator)           \
    X(UnaryOperator)            \
    X(LoadValue)                \
    X(StoreValue)               \
    X(LoadProperty)             \
    X(LoadNamedProperty)        \
    X(StoreProperty)            \
    X(StoreNamedProperty)       \
    X(Spread)                   \
    X(Function)                 \
    X(If)                       \
    X(IfTrue)                   \
    X(IfFalse)                  \
    X(Merge)                    \
    X(Phi)                      \
    X(TypeCast)                 \
    X(Switch)                   \
    X(Case)                     \
    X(Break)                    \
    X(Continue)                 \
    X(Loop)                     \
    X(ForOfLoop)                \
    X(Argument)

#define X(GRAPH_NODE_TYPE) GRAPH_NODE_TYPE,
enum class GraphNodeType : uint8_t {
    Invalid,
    GRAPH_NODE_TYPE_LIST(X)
};
#undef X

#endif // TYPE_H
