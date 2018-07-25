#include "basicblock.hpp"
#include "graph/graph.hpp"
#include <cassert>

BasicBlock::BasicBlock(Graph &graph, uint16_t selfIndex, std::vector<uint16_t> prevs)
    : prevs{prevs}, graph{graph}, selfIndex{selfIndex}, next{0}, newest{0}, sealed{false}
{
}

uint16_t BasicBlock::getSelfId()
{
    return selfIndex;
}

const std::vector<uint16_t>& BasicBlock::getPrevs()
{
    return prevs;
}

uint16_t BasicBlock::getNext()
{
    return next;
}

uint16_t BasicBlock::getNewest()
{
    return newest;
}

void BasicBlock::seal()
{
    // TODO: Add operands to all incomplete phis in this block

    sealed = true;
}

bool BasicBlock::isSealed()
{
    return sealed;
}

uint16_t BasicBlock::addNode(GraphNode &&node, bool control)
{
    newest = graph.addNode(std::move(node));
    if (control)
        next = newest;
    return newest;
}

uint16_t BasicBlock::addNode(GraphNode &&node, uint16_t prev, bool control)
{
    newest = graph.addNode(std::move(node), prev);
    if (control)
        next = newest;
    return newest;
}

uint16_t BasicBlock::addNode(GraphNode &&node, std::vector<uint16_t> &prevs, bool control)
{
    newest = graph.addNode(std::move(node), prevs);
    if (control)
        next = newest;
    return newest;
}

uint16_t BasicBlock::addPhi()
{
    assert(prevs.size() > 0);
    uint16_t merge = graph.getNode(graph.getBasicBlock(prevs[0]).getNext()).getNext(0);
    assert(graph.getNode(merge).getType() == GraphNodeType::Merge);

    // TODO: Fix prev/next
    uint16_t insertPoint = merge;
    while (graph.getNode(insertPoint).nextCount() == 1) {
        auto nextId = graph.getNode(insertPoint).getNext(0);
        auto next = graph.getNode(nextId);
        if (next.getType() == GraphNodeType::Phi)
            insertPoint = nextId;
        else
            break;
    }

    auto prevNode = graph.getNode(insertPoint);
    uint16_t phi = graph.addNode({GraphNodeType::Phi}, insertPoint);
    if (prevNode.nextCount()) {
        uint16_t prevNext = prevNode.getNext(0);
        graph.getNode(phi).addNext(prevNext);
        graph.getNode(prevNext).setPrev(0, phi);
    }
    prevNode.setNext(0, phi);

    return phi;
}

void BasicBlock::setNewest(uint16_t oldNode)
{
    newest = oldNode;
}

void BasicBlock::writeVariable(Identifier *declarationIdentifier, uint16_t valueNode)
{
    values[declarationIdentifier] = valueNode;
}

uint16_t* BasicBlock::readVariable(Identifier *declarationIdentifier)
{
    auto it = values.find(declarationIdentifier);
    if (it == values.end())
        return nullptr;
    return &it->second;
}
