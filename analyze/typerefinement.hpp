#ifndef TYPEPROPAGATION_H
#define TYPEPROPAGATION_H

struct ScopedTypes;
class Graph;
class GraphNode;

void refineTypes(Graph& graph, ScopedTypes& scope, GraphNode* node);

#endif // TYPEPROPAGATION_H
