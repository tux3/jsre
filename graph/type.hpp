#ifndef TYPE_H
#define TYPE_H

#include <cstdint>

#define GRAPH_NODE_TYPE_LIST(X) \
    X(Start)                    \
    X(End)                      \
    X(Return)                   \
    X(Literal)                  \
    X(BinaryOperator)           \
    X(UnaryOperator)            \
    X(LoadValue)                \
    X(StoreValue)               \
    X(LoadProperty)             \
    X(LoadNamedProperty)        \
    X(StoreProperty)            \
    X(StoreNamedProperty)       \
    X(If)                       \
    X(IfTrue)                   \
    X(IfFalse)                  \
    X(Merge)                    \
    X(Phi)

#define X(GRAPH_NODE_TYPE) GRAPH_NODE_TYPE,
enum class GraphNodeType : uint8_t {
    Invalid,
    GRAPH_NODE_TYPE_LIST(X)
};
#undef X

#endif // TYPE_H
