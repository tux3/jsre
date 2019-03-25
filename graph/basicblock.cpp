#include "analyze/astqueries.hpp"
#include "analyze/identresolution.hpp"
#include "ast/ast.hpp"
#include "graph/basicblock.hpp"
#include "graph/graph.hpp"
#include "graph/graphbuilder.hpp"
#include <cassert>
#include <algorithm>
#include <vector>

BasicBlock::BasicBlock(Graph &graph, uint16_t selfIndex, const LexicalBindings &scope, bool shouldHoist, std::vector<uint16_t> prevs)
    : prevs{prevs}, scope{scope}, graph{graph}, selfIndex{selfIndex}, next{0}, newest{0}, sealed{false}, filled{false}
{
    // Hoist bindings in the graph
    // For this to work, we have to make sure BBs only have the same scope if the earliest one is reachable from the other ones,
    // and only hoist bindings for that scope into the earliest BB (track already hoisted scopes in a set in Graph)
    if (!shouldHoist)
        return;
    for (const auto& [name, declId] : scope.localDeclarations) {
        // Skip parameters, instead of hoisting them as variables we use a LoadParameter node
        if (!isChildOf(declId, *graph.getFun().getBody()))
            continue;

        // Functions and classes are special, they get initialized during hoisting
        AstNode* decl = declId->getParent();
        uint16_t value;
        if (isFunctionNode(*decl) && ((Function*)decl)->getId() == declId)
            value = addNode({GraphNodeType::Function, decl}, false);
        else if (decl->getType() == AstNodeType::ClassDeclaration && ((ClassDeclaration*)decl)->getId() == declId)
            value = addNode({GraphNodeType::Class, decl}, false);
        else
            value = graph.getUndefinedNode();
        writeVariable(declId, value);
    }
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

#include "ast/ast.hpp"
#include "utils/reporting.hpp"

uint16_t BasicBlock::readNonlocalVariable(Identifier &declIdentifier)
{
    if (uint16_t* existingVar = readVariable(&declIdentifier))
        return *existingVar;

    // NOTE: This assert is overzealous, undeclared variables can trigger it (bug so will our bugs!)
    //assert(getPrevs().size());

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

uint16_t BasicBlock::addPhi(std::vector<uint16_t>&& inputs)
{
    assert(prevs.size() > 0);
    BasicBlock* prevBlock = &graph.getBasicBlock(prevs[0]);
    // Skip empty blocks that stole the previous block's next, we don't want to insert a phi in there.
    while (prevBlock->getPrevs().size() == 1 && prevBlock->next == graph.getBasicBlock(prevBlock->getPrevs()[0]).next) {
        assert(prevBlock->getPrevs().size() == 1);
        prevBlock = &graph.getBasicBlock(prevBlock->getPrevs()[0]);
    }
    uint16_t merge = graph.getNode(prevBlock->getNext()).getNext(0);
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

    // If we're appending a node in a block, we need to update its next.
    // If there's an empty block right after this one, it shares our next, so we need to update it too
    if (insertPoint == next) {
        next = newest = phi;
        for (uint16_t blockIndex = selfIndex+1; blockIndex < graph.blockCount(); ++blockIndex) {
            auto& block = graph.getBasicBlock(blockIndex);
            if (block.next == insertPoint) {
                assert(block.prevs.size() == 1 && block.prevs[0] == selfIndex); // Empty blocks should only ever have us as prev
                block.next = block.newest = phi;
            }
        }
    }
    return phi;
}

uint16_t BasicBlock::addIncompletePhi(Identifier& id)
{
    uint16_t phi = addPhi({});
    incompletePhis.push_back({&id, phi});
    return phi;
}

const LexicalBindings &BasicBlock::getScope() const
{
    return scope;
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
