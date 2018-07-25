#include "graph.hpp"
#include <utility>
#include <stdexcept>

const char* GraphNode::getTypeName() const {
    if (type == GraphNodeType::Invalid)
        return "Invalid";
    #define X(GRAPH_NODE_TYPE) \
        else if (type == GraphNodeType::GRAPH_NODE_TYPE) \
            return #GRAPH_NODE_TYPE;
    GRAPH_NODE_TYPE_LIST(X)
    #undef X
    else
            throw new std::runtime_error("Unknown graph node type "+std::to_string((int)type));
}

AstNode *GraphNode::getAstReference() const
{
    return astReference;
}

Graph::Graph()
{
    nodes.emplace_back(GraphNodeType::Start);
    BasicBlock& block = blocks.emplace_back(*this, 0);
    block.seal();
}

uint16_t Graph::size() const
{
    return static_cast<uint16_t>(nodes.size());
}

const GraphNode &Graph::getNode(uint16_t n) const
{
    return nodes[n];
}

GraphNode &Graph::getNode(uint16_t n)
{
    return nodes[n];
}

uint16_t Graph::addNode(GraphNode &&node)
{
    uint16_t newIndex = (uint16_t)nodes.size();
    nodes.emplace_back(std::move(node));
    return newIndex;
}

uint16_t Graph::addNode(GraphNode &&node, uint16_t prev)
{
    uint16_t newIndex = (uint16_t)nodes.size();
    node.addPrev(prev);
    nodes.emplace_back(std::move(node));
    nodes[prev].addNext(newIndex);
    return newIndex;
}

uint16_t Graph::addNode(GraphNode &&node, std::vector<uint16_t>& prevs)
{
    uint16_t newIndex = (uint16_t)nodes.size();
    for (const auto& prev : prevs) {
        node.addPrev(prev);
        nodes[prev].addNext(newIndex);
    }
    nodes.emplace_back(std::move(node));
    return newIndex;
}

uint16_t Graph::blockCount() const
{
    return blocks.size();
}

const BasicBlock &Graph::getBasicBlock(uint16_t n) const
{
    return blocks[n];
}

BasicBlock &Graph::getBasicBlock(uint16_t n)
{
    return blocks[n];
}

BasicBlock& Graph::addBasicBlock(std::vector<uint16_t> prevs)
{
    uint16_t newIndex = (uint16_t)blocks.size();
    return blocks.emplace_back(*this, newIndex, move(prevs));
}

GraphNode::GraphNode(GraphNodeType type, AstNode* astReference)
    : astReference{astReference}, type{type}
{
}

GraphNode::GraphNode(GraphNodeType type, uint16_t input, AstNode *astReference)
    : astReference{astReference}, type{type}
{
   inputs.push_back(input);
}

GraphNode::GraphNode(GraphNodeType type, std::vector<uint16_t>&& inputs, AstNode* astReference)
    : inputs{std::move(inputs)}, astReference{astReference}, type{type}
{
}

GraphNodeType GraphNode::getType() const
{
    return type;
}

size_t GraphNode::inputCount() const
{
    return inputs.size();
}

size_t GraphNode::prevCount() const
{
    return prev.size();
}

size_t GraphNode::nextCount() const
{
    return next.size();
}

uint16_t GraphNode::getInput(uint16_t n) const
{
    return inputs[n];
}

uint16_t GraphNode::getPrev(uint16_t n) const
{
    return prev[n];
}

uint16_t GraphNode::getNext(uint16_t n) const
{
    return next[n];
}

void GraphNode::addInput(uint16_t n)
{
    inputs.push_back(n);
}

void GraphNode::addPrev(uint16_t n)
{
    prev.push_back(n);
}

void GraphNode::addNext(uint16_t n)
{
    next.push_back(n);
}

void GraphNode::setPrev(uint16_t idx, uint16_t newValue)
{
    prev[idx] = newValue;
}

void GraphNode::setNext(uint16_t idx, uint16_t newValue)
{
    next[idx] = newValue;
}
