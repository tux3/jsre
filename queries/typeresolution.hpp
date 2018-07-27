#ifndef TYPERESOLUTION_HPP
#define TYPERESOLUTION_HPP

#include "queries/types.hpp"

class Graph;
class GraphNode;

TypeInfo resolveAstAnnotationType(AstNode& node);
TypeInfo resolveAstNodeType(AstNode& node);
TypeInfo resolveReturnType(Function& fun);
TypeInfo resolveNodeType(Graph& graph, GraphNode* node);

#endif // TYPERESOLUTION_HPP
