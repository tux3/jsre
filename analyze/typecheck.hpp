#ifndef TYPECHECK_HPP
#define TYPECHECK_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <memory>
#include <variant>

class Module;
class AstNode;
class Function;
class Graph;
class GraphNode;

void typecheckGraph(Graph& graph);
void runTypechecks(Module& module);

#endif // TYPECHECK_HPP
