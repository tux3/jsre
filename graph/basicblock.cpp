#include "basicblock.hpp"
#include "graph/graph.hpp"
#include <cassert>
#include <algorithm>
#include <vector>

BasicBlock::BasicBlock(Graph &graph, uint16_t selfIndex, std::vector<uint16_t> prevs)
    : prevs{prevs}, graph{graph}, selfIndex{selfIndex}, next{0}, newest{0}, sealed{false}, filled{false}
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

uint16_t BasicBlock::readNonlocalVariable(Identifier &declIdentifier)
{
    if (uint16_t* existingVar = readVariable(&declIdentifier))
        return *existingVar;

    // NOTE: This assert is overzealous, undeclared variables can trigger it (bug so will our bugs!)
    assert(getPrevs().size());

    uint16_t result;
    if (!isSealed()) {
        result = addIncompletePhi(declIdentifier);
    } else if (prevs.size() == 1) {
        result = graph.getBasicBlock(prevs[0]).readNonlocalVariable(declIdentifier);
    } else {
        result = completeSimplePhi(declIdentifier);
    }
    writeVariable(&declIdentifier, result);
    return result;
}


uint16_t BasicBlock::completeSimplePhi(Identifier &declIdentifier)
{
    bool trivial = true;
    writeVariable(&declIdentifier, 0); // Set placeholder to break loops
    std::vector<uint16_t> inputs;
    for (auto prevId : prevs) {
        BasicBlock& prevBlock = graph.getBasicBlock(prevId);
        uint16_t newInput;
        if (uint16_t* existingVar = prevBlock.readVariable(&declIdentifier)) {
            newInput = *existingVar;
        } else {
            newInput = prevBlock.readNonlocalVariable(declIdentifier);
        }
        if (!newInput)
            continue;
        if (!inputs.empty() && inputs.back() != newInput)
            trivial = false;
        inputs.push_back(newInput); // If we can't remove the phi entirely, we need to keep every input (or it breaks wrt merges)
    }

    if (inputs.empty())
        return 0;
    if (trivial)
        return inputs[0];
    return addPhi(move(inputs));
}

void BasicBlock::seal()
{
    assert(!isSealed());

    for (const auto& incomplete : incompletePhis) {
        Identifier* identifierDecl = incomplete.first;
        uint16_t phi = incomplete.second;
        assert(graph.getNode(phi).getType() == GraphNodeType::Phi);

        for (auto prev : prevs) {
            uint16_t op = graph.getBasicBlock(prev).readNonlocalVariable(*identifierDecl);
            auto& phiNode = graph.getNode(phi);
            phiNode.addInput(op); // If we can't remove the phi entirely, we need to keep every input (or it breaks wrt merges)
        }

        // TODO: We could try to remove some trivial phis here. More work, but could be worth it to help type resolution
    }

    incompletePhis.clear();
    incompletePhis.shrink_to_fit();
    sealed = true;
}

bool BasicBlock::isSealed()
{
    return sealed;
}

void BasicBlock::setFilled()
{
    filled = true;
}

bool BasicBlock::isFilled()
{
    return filled;
}

void BasicBlock::addPrevBlock(uint16_t prev)
{
    assert(!isSealed());
    assert(std::find(prevs.begin(), prevs.end(), prev) == prevs.end());
    prevs.push_back(prev);
}

uint16_t BasicBlock::addNode(GraphNode &&node, bool control)
{
    assert(!isFilled());

    newest = graph.addNode(std::move(node));
    if (control)
        next = newest;
    return newest;
}

uint16_t BasicBlock::addNode(GraphNode &&node, uint16_t prev, bool control)
{
    assert(!isFilled());

    newest = graph.addNode(std::move(node), prev);
    if (control)
        next = newest;
    return newest;
}

uint16_t BasicBlock::addNode(GraphNode &&node, std::vector<uint16_t> &prevs, bool control)
{
    assert(!isFilled());

    newest = graph.addNode(std::move(node), prevs);
    if (control)
        next = newest;
    return newest;
}

#include "reporting.hpp"

uint16_t BasicBlock::addPhi(std::vector<uint16_t>&& inputs)
{
    assert(prevs.size() > 0);
    uint16_t merge = graph.getNode(graph.getBasicBlock(prevs[0]).getNext()).getNext(0);
    assert(graph.getNode(merge).getType() == GraphNodeType::Merge);

    uint16_t insertPoint = merge;
    while (graph.getNode(insertPoint).nextCount() == 1) {
        auto nextId = graph.getNode(insertPoint).getNext(0);
        auto& next = graph.getNode(nextId);
        if (next.getType() == GraphNodeType::Phi)
            insertPoint = nextId;
        else
            break;
    }

    uint16_t phi = graph.addNode({GraphNodeType::Phi, move(inputs)});
    auto& prevNode = graph.getNode(insertPoint);
    auto& phiNode = graph.getNode(phi);
    phiNode.addPrev(insertPoint);
    if (prevNode.nextCount()) {
        assert(prevNode.nextCount() == 1);
        uint16_t prevNext = prevNode.getNext(0);
        phiNode.addNext(prevNext);
        graph.getNode(prevNext).replacePrev(insertPoint, phi);
        prevNode.setNext(0, phi);
    } else {
        prevNode.addNext(phi);
    }

    if (insertPoint == next)
        next = newest = phi;
    return phi;
}

uint16_t BasicBlock::addIncompletePhi(Identifier& id)
{
    uint16_t phi = addPhi({});
    incompletePhis.push_back({&id, phi});
    return phi;
}

void BasicBlock::setNewest(uint16_t oldNode)
{
    newest = oldNode;
}

void BasicBlock::setNext(uint16_t oldNode)
{
    next = oldNode;
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
