#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>

#include "graph/basicblock.hpp"
#include "queries/types.hpp"
#include "graph/type.hpp"

class AstNode;
class GraphNode;
class GraphStart;
struct LexicalBindings;

class Graph
{
public:
    Graph(Function& fun, const LexicalBindings& scope);
    Function& getFun() const;
    uint16_t size() const;
    const GraphNode &getNode(uint16_t n) const;
    GraphNode &getNode(uint16_t n);
    uint16_t getUndefinedNode();
    uint16_t addNode(GraphNode&& node);
    uint16_t addNode(GraphNode&& node, uint16_t prev);
    uint16_t addNode(GraphNode &&node, const std::vector<uint16_t> &prevs);

    uint16_t blockCount() const;
    const BasicBlock& getBasicBlock(uint16_t n) const;
    BasicBlock& getBasicBlock(uint16_t n);
    BasicBlock& addBasicBlock(std::vector<uint16_t> prevs, const LexicalBindings& scope, bool shouldHoist);

public:
    std::unordered_map<const GraphNode*, TypeInfo> nodeTypes;

private:
    std::vector<GraphNode> nodes;
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    Function& fun;
};

class GraphNode
{
public:
    GraphNode(GraphNodeType type, AstNode* astReference = nullptr);
    GraphNode(GraphNodeType type, uint16_t input, AstNode* astReference = nullptr);
    GraphNode(GraphNodeType type, std::vector<uint16_t> &&inputs, AstNode* astReference = nullptr);
    GraphNode(const GraphNode& other) = delete;
    GraphNode(GraphNode&& other) = default;
    virtual ~GraphNode() = default;

    GraphNodeType getType() const;
    const char* getTypeName() const;
    AstNode* getAstReference() const;

    size_t inputCount() const;
    size_t prevCount() const;
    size_t nextCount() const;

    uint16_t getInput(uint16_t n) const;
    uint16_t getPrev(uint16_t n) const;
    uint16_t getNext(uint16_t n) const;

    void addInput(uint16_t n);
    void addPrev(uint16_t n);
    void addNext(uint16_t n);

    void setPrev(uint16_t idx, uint16_t newValue);
    void setNext(uint16_t idx, uint16_t newValue);

    void replacePrev(uint16_t oldValue, uint16_t newValue);

private:
    // TODO: Switch to small vecs, or anything else that doesn't use 24 bytes + 1 allocation per object...
    std::vector<uint16_t> inputs; // Data dependencies
    std::vector<uint16_t> prevs, nexts; // Control dependencies
    // TODO: On x86_64 we can save 7 bytes by putting the node type in the expression pointer (canonical addresses and all).
    AstNode* astReference;
    GraphNodeType type;
};

#endif // GRAPH_HPP
