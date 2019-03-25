#include "graph.hpp"
#include "analyze/identresolution.hpp"
#include <utility>
#include <stdexcept>
#include <cassert>

Graph::Graph(Function &fun, const LexicalBindings &scope)
    : fun{fun}
{
    assert(scope.code == (AstNode*)&fun);
    nodes.emplace_back(GraphNodeType::Start);
    nodes.emplace_back(GraphNodeType::Undefined); // The Undefined literal is used so often, we hardcode it once.
}

Function &Graph::getFun() const
{
    return fun;
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

uint16_t Graph::getUndefinedNode()
{
    return 1; // We hardcode node 1 as the Undefined literal node.
}

uint16_t Graph::addNode(GraphNode &&node)
{
    uint16_t newIndex = (uint16_t)nodes.size();
    nodes.emplace_back(std::move(node));
    return newIndex;
}

uint16_t Graph::addNode(GraphNode &&node, uint16_t prev)
{
    assert(prev != 0 || !nodes[0].nextCount());

    uint16_t newIndex = (uint16_t)nodes.size();
    node.addPrev(prev);
    nodes.emplace_back(std::move(node));
    nodes[prev].addNext(newIndex);
    return newIndex;
}

uint16_t Graph::addNode(GraphNode &&node, const std::vector<uint16_t>& prevs)
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
    return static_cast<uint16_t>(blocks.size());
}

const BasicBlock &Graph::getBasicBlock(uint16_t n) const
{
    return *blocks[n];
}

BasicBlock &Graph::getBasicBlock(uint16_t n)
{
    return *blocks[n];
}

BasicBlock& Graph::addBasicBlock(std::vector<uint16_t> prevs, const LexicalBindings& scope, bool shouldHoist)
{
    uint16_t newIndex = (uint16_t)blocks.size();
    return *blocks.emplace_back(std::make_unique<BasicBlock>(*this, newIndex, scope, shouldHoist, move(prevs)));
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
    return prevs.size();
}

size_t GraphNode::nextCount() const
{
    return nexts.size();
}

uint16_t GraphNode::getInput(uint16_t n) const
{
    return inputs[n];
}

uint16_t GraphNode::getPrev(uint16_t n) const
{
    return prevs[n];
}

uint16_t GraphNode::getNext(uint16_t n) const
{
    return nexts[n];
}

void GraphNode::addInput(uint16_t n)
{
    inputs.push_back(n);
}

void GraphNode::addPrev(uint16_t n)
{
    prevs.push_back(n);
}

void GraphNode::addNext(uint16_t n)
{
    nexts.push_back(n);
}

void GraphNode::setPrev(uint16_t idx, uint16_t newValue)
{
    prevs[idx] = newValue;
}

void GraphNode::setNext(uint16_t idx, uint16_t newValue)
{
    nexts[idx] = newValue;
}

void GraphNode::replacePrev(uint16_t oldValue, uint16_t newValue)
{
    for (uint16_t& prev : prevs) {
        if (prev == oldValue) {
            prev = newValue;
            return;
        }
    }
    assert(false && "Prev not found!");
}
