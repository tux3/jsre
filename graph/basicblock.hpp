#ifndef BASICBLOCK_HPP
#define BASICBLOCK_HPP

#include <cstdint>
#include <vector>
#include <unordered_map>

class Graph;
class GraphNode;
class Identifier;

class BasicBlock
{
public:
    BasicBlock(Graph& graph, uint16_t selfIndex, std::vector<uint16_t> prevs = {});

    const std::vector<uint16_t> &getPrevs();
    uint16_t getSelfId();
    uint16_t getNext();
    uint16_t getNewest();

    void seal();
    bool isSealed();

    uint16_t addNode(GraphNode&& node, bool control = false);
    uint16_t addNode(GraphNode&& node, uint16_t prev, bool control = true);
    uint16_t addNode(GraphNode&& node, std::vector<uint16_t> &prevs, bool control = true);
    uint16_t addPhi();

    // Instead of adding a duplicate of a node that already exists, users may reuse an existing node and set it as newest
    void setNewest(uint16_t oldNode);

    // declarationIdentifier must be the identifier of the original declaration. A valueNode of 0 means undefined
    void writeVariable(Identifier* declarationIdentifier, uint16_t valueNode);
    // declarationIdentifier must be the identifier of the original declaration. Returns nullptr if the variable doesn't exist.
    uint16_t *readVariable(Identifier* declarationIdentifier);

private:
    std::unordered_map<Identifier*, uint16_t> values;
    std::vector<uint16_t> prevs; // Index of the previous basic blocks

    Graph& graph;
    uint16_t selfIndex; // Index of the basic block in the graph's list
    uint16_t next; // Last control node added to the block, new control nodes will have this as prev
    uint16_t newest; // Latest node added through the block, different from next for non-control node

    bool sealed;
};

#endif // BASICBLOCK_HPP
