#ifndef TYPECHECK_HPP
#define TYPECHECK_HPP

#include "queries/types.hpp"
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <variant>

class Module;
class AstNode;
class Function;
class Graph;
class GraphNode;

struct ScopedTypes
{
    std::unordered_map<GraphNode const*, TypeInfo> types;
    std::unordered_set<ScopedTypes*> prevs;
    unsigned visited = 0;
};

void runTypechecks(Module& module);

#endif // TYPECHECK_HPP
