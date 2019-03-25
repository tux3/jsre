#include "graphbuilder.hpp"
#include "basicblock.hpp"
#include "ast/walk.hpp"
#include "analyze/astqueries.hpp"
#include "analyze/identresolution.hpp"
#include "utils/reporting.hpp"
#include "module/module.hpp"
#include <cassert>

using namespace std;

GraphBuilder::GraphBuilder(Function &fun)
    : fun{fun}, parentModule{fun.getParentModule()}
{
}

std::unique_ptr<Graph> GraphBuilder::buildFromAst()
{
    const LexicalBindings* funScope = &fun.getParentModule().getScopeChain();
    function<void(AstNode*)> walkFunScopeChain;
    walkFunScopeChain = [&](AstNode* node){
        if (AstNode* parent = node->getParent())
            walkFunScopeChain(parent);
        funScope = &funScope->scopeForChildNode(node);
    };
    walkFunScopeChain(&fun);
    assert(funScope->code == &fun);

    graph = make_unique<Graph>(fun, *funScope);
    auto& rootBlock = addBasicBlock({}, *funScope);
    rootBlock.seal();

    AstNode* body = fun.getBody();
    BasicBlock* bodyBlock = &addBasicBlock({graph->getBasicBlock(0).getSelfId()}, graph->getBasicBlock(0).getScope().scopeForChildNode(body));
    bodyBlock->seal();
    auto* block = processAstNode(bodyBlock, *body);
    if (body->getType() != AstNodeType::BlockStatement)
        block->addNode({GraphNodeType::Return, block->getNewest()}, block->getNext());

    vector<uint16_t> leaves;
    for (uint16_t i=0; i<graph->size(); ++i) {
        GraphNode& node = graph->getNode(i);
        if (node.prevCount() > 0 && node.nextCount() == 0) {
            assert(node.getType() != GraphNodeType::Break); // If this happens, we forgot to tie up pending breaks
            assert(node.getType() != GraphNodeType::Continue); // If this happens, we forgot to tie up pending continues
            assert(node.getType() != GraphNodeType::PrepareException); // We forgot a throw that was supposed to be caught
            leaves.push_back(i);
        }
        assert(node.getType() != GraphNodeType::Phi || node.inputCount() > 0 );
        for (uint16_t j=0; j<node.inputCount(); ++j) {
            if (node.getInput(j) == 0)
                trace(fun, "About to fail graphbuilder assert for function:\n"+fun.getSourceString());
            assert(node.getInput(j) != 0);
        }
    }
    if (leaves.empty()) {
        if (graph->size() == 1)
            leaves.push_back(0);
    }
    if (!leaves.empty()) // A function can have no exit control flow at all (e.g. "do {continue} while (0)")
        graph->addNode({GraphNodeType::End}, leaves);

    for (uint16_t i=0; i<graph->blockCount(); ++i)
        if (!graph->getBasicBlock(i).isSealed())
            assert(false && "Graph built but not all blocks are sealed!");

    assert(pendingBreakBlocks.empty());
    assert(pendingContinueBlocks.empty());

    return std::move(graph);
}

Graph &GraphBuilder::getGraph()
{
    return *graph;
}

BasicBlock &GraphBuilder::addBasicBlock(std::vector<uint16_t> prevs, const LexicalBindings &scope)
{
    bool shouldHoist = hoistedScopes.insert(&scope).second;
    return graph->addBasicBlock(move(prevs), scope, shouldHoist);
}

void GraphBuilder::writeVariableById(BasicBlock &block, Identifier &id, uint16_t value)
{
    const auto& resolvedIds = parentModule.getResolvedLocalIdentifiers();
    auto declarationIt = resolvedIds.find((Identifier*)&id);
    Identifier* foundDeclId = declarationIt != resolvedIds.end() ? declarationIt->second : nullptr;
    if (isChildOf(foundDeclId, *fun.getBody())) {
        block.writeVariable(foundDeclId, value);
        return;
    }

    for (auto param : fun.getParams()) {
        if (isChildOf(foundDeclId, *param)) {
            block.addNode({GraphNodeType::StoreParameter, foundDeclId}, block.getNext());
            return;
        }
    }

    block.addNode({GraphNodeType::StoreValue, value, &id}, block.getNext());
}

BasicBlock* GraphBuilder::processAstNode(BasicBlock* block, AstNode &node)
{
    switch (node.getType()) {
    case AstNodeType::EmptyStatement:
        break;
    case AstNodeType::BlockStatement: {
        BasicBlock *innerBlock = &addBasicBlock({block->getSelfId()}, block->getScope().scopeForChildNode(&node));
        block->setFilled();
        innerBlock->setNext(block->getNext());
        innerBlock->seal();
        node.applyChildren([&](AstNode* child) {
            innerBlock = processAstNode(innerBlock, *child);
            if (child->getType() == AstNodeType::ReturnStatement || child->getType() == AstNodeType::ThrowStatement)
                return false;
            if (innerBlock->isFilled()) // If there is any more code it must be unreachable
                return false;
            return true;
        });
        if (innerBlock->isFilled()) {
            block = innerBlock; // We're not exiting the scope, but that's okay since the block is filled
        } else {
            innerBlock->setFilled();
            block = &addBasicBlock({innerBlock->getSelfId()}, block->getScope());
            block->setNext(innerBlock->getNext());
            block->seal();
        }
        break;
    }
    case AstNodeType::FunctionDeclaration:
    case AstNodeType::FunctionExpression:
    case AstNodeType::ArrowFunctionExpression:
    case AstNodeType::ClassMethod:
    case AstNodeType::ClassPrivateMethod:
    case AstNodeType::ObjectMethod:
        block = processFunctionNode(block, (Function&)node);
        break;
    case AstNodeType::ReturnStatement:
        if (auto arg = ((ReturnStatement&)node).getArgument()) {
            block = processAstNode(block, *arg);
            block->addNode({GraphNodeType::Return, block->getNewest()}, block->getNext());
        } else {
            block->addNode({GraphNodeType::Return}, block->getNext());
        }
        block->setFilled();
        break;
    case AstNodeType::BreakStatement:
        block = processBreakStatement(block, (BreakStatement&)node);
        break;
    case AstNodeType::ContinueStatement:
        block = processContinueStatement(block, (ContinueStatement&)node);
        break;
    case AstNodeType::AwaitExpression:
        block = processAstNode(block, *((AwaitExpression&)node).getArgument());
        block->addNode({GraphNodeType::Await, block->getNewest()}, block->getNext());
        break;
    case AstNodeType::ExpressionStatement:
        block = processAstNode(block, *((ExpressionStatement&)node).getExpression());
        break;
    case AstNodeType::UnaryExpression:
        block = processUnaryExprNode(block, (UnaryExpression&)node);
        break;
    case AstNodeType::BinaryExpression:
        block = processBinaryExprNode(block, (BinaryExpression&)node);
        break;
    case AstNodeType::UpdateExpression:
        block = processUpdateExprNode(block, (UpdateExpression&)node);
        break;
    case AstNodeType::LogicalExpression:
        block = processLogicalExprNode(block, (LogicalExpression&)node);
        break;
    case AstNodeType::MemberExpression:
        block = processMemberExprNode(block, (MemberExpression&)node);
        break;
    case AstNodeType::AssignmentExpression:
        block = processAssignmentExprNode(block, (AssignmentExpression&)node);
        break;
    case AstNodeType::NewExpression:
    case AstNodeType::CallExpression:
        block = processCallExprNode(block, (CallExpression&)node);
        break;
    case AstNodeType::ArrayExpression:
        block = processArrayExprNode(block, (ArrayExpression&)node);
        break;
    case AstNodeType::ObjectExpression:
        block = processObjectExprNode(block, (ObjectExpression&)node);
        break;
    case AstNodeType::ObjectProperty:
        block = processObjectPropNode(block, (ObjectProperty&)node);
        break;
    case AstNodeType::SpreadElement:
        block = processSpreadElemNode(block, (SpreadElement&)node);
        break;
    case AstNodeType::NullLiteral:
    case AstNodeType::NumericLiteral:
    case AstNodeType::BooleanLiteral:
    case AstNodeType::StringLiteral:
    case AstNodeType::RegExpLiteral:
        block->addNode({GraphNodeType::Literal, &node});
        break;
    case AstNodeType::ThisExpression:
        block->addNode({GraphNodeType::This, &node});
        break;
    case AstNodeType::Super:
        block->addNode({GraphNodeType::Super, &node});
        break;
    case AstNodeType::TemplateLiteral:
        block = processTemplateLiteralNode(block, (TemplateLiteral&)node);
        break;
    case AstNodeType::VariableDeclaration:
        block = processVariableDeclarationNode(block, (VariableDeclaration&)node);
        break;
    case AstNodeType::Identifier:
        block = processIdentifierNode(block, (Identifier&)node);
        break;
    case AstNodeType::IfStatement:
        block = processIfStatement(block, (IfStatement&)node);
        break;
    case AstNodeType::WhileStatement:
        block = processWhileStatement(block, (WhileStatement&)node);
        break;
    case AstNodeType::DoWhileStatement:
        block = processDoWhileStatement(block, (DoWhileStatement&)node);
        break;
    case AstNodeType::ForStatement:
        block = processForStatement(block, (ForStatement&)node);
        break;
    case AstNodeType::ForOfStatement:
        block = processForOfStatement(block, (ForOfStatement&)node);
        break;
    case AstNodeType::ConditionalExpression:
        block = processConditionalExpression(block, (ConditionalExpression&)node);
        break;
    case AstNodeType::ThrowStatement:
        block = processThrowStatement(block, (ThrowStatement&)node);
        break;
    case AstNodeType::TryStatement:
        block = processTryStatement(block, (TryStatement&)node);
        break;
    case AstNodeType::TypeCastExpression:
        block = processTypeCastExpr(block, (TypeCastExpression&)node);
        break;
    case AstNodeType::SwitchStatement:
        block = processSwitchStatement(block, (SwitchStatement&)node);
        break;
    default:
        trace(node, "GraphBuilder cannot handle "s+node.getTypeName()+" AST nodes!");
        throw runtime_error("GraphBuilder cannot handle "s+node.getTypeName()+" AST nodes!");
    }

    return block;
}

BasicBlock *GraphBuilder::processIfStatement(BasicBlock *block, IfStatement &node)
{
    block = processAstNode(block, *node.getTest());
    block->addNode({GraphNodeType::If, block->getNewest()}, block->getNext());
    uint16_t prevNodeId = block->getNext();
    uint16_t prevBlockId = block->getSelfId();
    vector<uint16_t> mergePrevs;
    vector<uint16_t> mergePrevBlocks;

    // Add then block & node
    BasicBlock *consequent = &addBasicBlock({prevBlockId}, block->getScope().scopeForChildNode(node.getConsequent()));
    consequent->seal();
    consequent->addNode({GraphNodeType::IfTrue}, prevNodeId);
    consequent = processAstNode(consequent, *node.getConsequent());
    if (!consequent->isFilled()) {
        mergePrevs.push_back(consequent->getNext());
        mergePrevBlocks.push_back(consequent->getSelfId());
    }

    // Add alternate block (and node, if any)
    AstNode* alternateNode = node.getAlternate();
    BasicBlock *alternate = &addBasicBlock({prevBlockId}, block->getScope().scopeForChildNode(alternateNode));
    alternate->seal();
    alternate->addNode({GraphNodeType::IfFalse}, prevNodeId);
    if (alternateNode)
        alternate = processAstNode(alternate, *alternateNode);
    if (!alternate->isFilled()) {
        mergePrevs.push_back(alternate->getNext());
        mergePrevBlocks.push_back(alternate->getSelfId());
    }

    // Create block for merge and add merge node
    BasicBlock *mergeBlock = &addBasicBlock(move(mergePrevBlocks), block->getScope());
    if (!mergePrevs.empty()) {
        mergeBlock->addNode({GraphNodeType::Merge}, mergePrevs);
        mergeBlock->seal();
    } else {
        mergeBlock->seal();
        mergeBlock->setFilled();
    }

    return mergeBlock;
}

BasicBlock *GraphBuilder::processWhileStatement(BasicBlock *block, WhileStatement &node)
{
    // Add new block for loop header
    uint16_t prevNodeId = block->getNext();
    BasicBlock *headerBlock = &addBasicBlock({block->getSelfId()}, block->getScope());
    uint16_t headerStartBlockId = headerBlock->getSelfId();
    uint16_t headerMergeNode = headerBlock->addNode({GraphNodeType::Merge}, prevNodeId);
    headerBlock = processAstNode(headerBlock, *node.getTest());
    uint16_t headerLoopNode = headerBlock->addNode({GraphNodeType::Loop, headerBlock->getNewest()}, headerBlock->getNext());
    uint16_t headerEndBlockId = headerBlock->getSelfId();

    // Add body block & node
    pendingBreakBlocks.emplace_back();
    pendingContinueBlocks.emplace_back();
    BasicBlock *body = &addBasicBlock({headerEndBlockId}, block->getScope().scopeForChildNode(node.getBody()));
    body->seal();
    body->addNode({GraphNodeType::IfTrue}, headerLoopNode);
    body = processAstNode(body, *node.getBody());
    if (!body->isFilled()) {
        // Body jumps to the loop header (unless it diverged)
        graph->getNode(body->getNext()).addNext(headerMergeNode);
        graph->getNode(headerMergeNode).addPrev(body->getNext());
        headerBlock = &graph->getBasicBlock(headerStartBlockId);
        headerBlock->addPrevBlock(body->getSelfId());
    }

    // Tie up any continue statements
    assert(!pendingContinueBlocks.empty());
    headerBlock = &graph->getBasicBlock(headerStartBlockId);
    for (uint16_t continueBlockId : pendingContinueBlocks.back()) {
        BasicBlock* continueBlock = &graph->getBasicBlock(continueBlockId);
        graph->getNode(continueBlock->getNext()).addNext(headerMergeNode);
        graph->getNode(headerMergeNode).addPrev(continueBlock->getNext());
        headerBlock->addPrevBlock(continueBlockId);
    }
    pendingContinueBlocks.pop_back();
    headerBlock->seal();

    // Create block for loop exit
    BasicBlock *exitBlock = &addBasicBlock({headerEndBlockId}, block->getScope());
    exitBlock->addNode({GraphNodeType::IfFalse}, headerLoopNode);
    exitBlock->seal();

    // Tie up any break statements and merge
    vector<uint16_t> mergePrevs;
    vector<uint16_t> mergePrevBlocks;
    assert(!pendingBreakBlocks.empty());
    for (uint16_t breakBlockId : pendingBreakBlocks.back()) {
        BasicBlock* breakBlock = &graph->getBasicBlock(breakBlockId);
        mergePrevBlocks.push_back(breakBlockId);
        mergePrevs.push_back(breakBlock->getNext());
    }
    pendingBreakBlocks.pop_back();

    // Create merge if we need to
    if (mergePrevs.empty()) {
        return exitBlock;
    } else {
        mergePrevs.push_back(exitBlock->getNext());
        mergePrevBlocks.push_back(exitBlock->getSelfId());
        BasicBlock *mergeBlock = &addBasicBlock({mergePrevBlocks}, block->getScope());
        mergeBlock->addNode({GraphNodeType::Merge}, mergePrevs);
        mergeBlock->seal();
        return mergeBlock;
    }
}

BasicBlock *GraphBuilder::processDoWhileStatement(BasicBlock *block, DoWhileStatement &node)
{
    // Add body block & node
    pendingBreakBlocks.emplace_back();
    pendingContinueBlocks.emplace_back();
    uint16_t prevNodeId = block->getNext();
    BasicBlock* body = &addBasicBlock({block->getSelfId()}, block->getScope().scopeForChildNode(node.getBody()));
    uint16_t bodyStartId = body->getSelfId();
    uint16_t bodyMergeNode = body->addNode({GraphNodeType::Merge}, prevNodeId);
    body = processAstNode(body, *node.getBody());

    BasicBlock* preMergeBlock;
    if (body->isFilled()) {
        preMergeBlock = body;
    } else {
        // Loop test
        body = processAstNode(body, *node.getTest());
        uint16_t loopNode = body->addNode({GraphNodeType::Loop, body->getNewest()}, body->getNext());
        uint16_t testEndBlockId = body->getSelfId();

        // Need a whole block just to jump back to body
        BasicBlock* ifTrueBlock = &addBasicBlock({testEndBlockId}, body->getScope());
        ifTrueBlock->seal();
        ifTrueBlock->addNode({GraphNodeType::IfTrue}, loopNode);
        graph->getNode(ifTrueBlock->getNext()).addNext(bodyMergeNode);
        graph->getNode(bodyMergeNode).addPrev(ifTrueBlock->getNext());
        graph->getBasicBlock(bodyStartId).addPrevBlock(ifTrueBlock->getSelfId());

        // Loop exit
        BasicBlock *exitBlock = &addBasicBlock({testEndBlockId}, block->getScope());
        exitBlock->addNode({GraphNodeType::IfFalse}, loopNode);
        exitBlock->seal();
        preMergeBlock = exitBlock;
    }

    // Tie up any continue statements
    assert(!pendingContinueBlocks.empty());
    body = &graph->getBasicBlock(bodyStartId);
    for (uint16_t continueBlockId : pendingContinueBlocks.back()) {
        BasicBlock* continueBlock = &graph->getBasicBlock(continueBlockId);
        graph->getNode(continueBlock->getNext()).addNext(bodyMergeNode);
        graph->getNode(bodyMergeNode).addPrev(continueBlock->getNext());
        body->addPrevBlock(continueBlockId);
    }
    pendingContinueBlocks.pop_back();
    body->seal();

    // Tie up any break statements and merge
    vector<uint16_t> mergePrevs;
    vector<uint16_t> mergePrevBlocks;
    assert(!pendingBreakBlocks.empty());
    for (uint16_t breakBlockId : pendingBreakBlocks.back()) {
        BasicBlock* breakBlock = &graph->getBasicBlock(breakBlockId);
        mergePrevBlocks.push_back(breakBlockId);
        mergePrevs.push_back(breakBlock->getNext());
    }
    pendingBreakBlocks.pop_back();

    // Create merge if we need to
    if (mergePrevs.empty()) {
        // The preMergeBlock is still in the loop's scope, we can't let anyone outside the loop add nodes in that scope, so make a new block if needed
        if (preMergeBlock->isFilled())
            return preMergeBlock;
        BasicBlock *scopeExitBlock = &addBasicBlock({preMergeBlock->getSelfId()}, block->getScope());
        // Steal the previous block's next, we "own" it now. (No need to take the newest, DoWhile is not an expression).
        preMergeBlock->setFilled();
        scopeExitBlock->setNext(preMergeBlock->getNext());
        scopeExitBlock->seal();
        return scopeExitBlock;
    } else {
        if (!preMergeBlock->isFilled()) {
            mergePrevs.push_back(preMergeBlock->getNext());
            mergePrevBlocks.push_back(preMergeBlock->getSelfId());
        }
        BasicBlock *mergeBlock = &addBasicBlock({mergePrevBlocks}, block->getScope());
        mergeBlock->addNode({GraphNodeType::Merge}, mergePrevs);
        mergeBlock->seal();
        return mergeBlock;
    }
}

BasicBlock *GraphBuilder::processForStatement(BasicBlock *block, ForStatement &node)
{
    const LexicalBindings& forScope = block->getScope().scopeForChildNode(&node);

    // Loop init goes first (if any)
    BasicBlock *initBlock = &addBasicBlock({block->getSelfId()}, forScope);
    block->setFilled();
    initBlock->setNext(block->getNext()); // Steal the prev block's next, since we may have an empty block (but we need the scope).
    initBlock->seal();
    if (AstNode* init = node.getInit())
        initBlock = processAstNode(initBlock, *init);
    initBlock->setFilled();

    // Add new block for loop merge and test
    BasicBlock *headerBlock = &addBasicBlock({initBlock->getSelfId()}, forScope);
    uint16_t headerStartBlockId = headerBlock->getSelfId();
    uint16_t headerMergeNode = headerBlock->addNode({GraphNodeType::Merge}, initBlock->getNext());
    uint16_t headerLoopNode;
    if (AstNode* test = node.getTest()) {
        headerBlock = processAstNode(headerBlock, *test);
        headerLoopNode = headerBlock->addNode({GraphNodeType::Loop, headerBlock->getNewest()}, headerBlock->getNext());
    } else {
        headerLoopNode = headerBlock->addNode({GraphNodeType::Loop}, headerBlock->getNext());
    }

    // Add body block & node
    pendingBreakBlocks.emplace_back();
    pendingContinueBlocks.emplace_back();
    BasicBlock *body = &addBasicBlock({headerBlock->getSelfId()}, headerBlock->getScope().scopeForChildNode(node.getBody()));
    body->seal();
    body->addNode({GraphNodeType::IfTrue}, headerLoopNode);
    body = processAstNode(body, *node.getBody());
    if (!body->isFilled()) {
        if (AstNode* update = node.getUpdate())
            body = processAstNode(body, *update);

        // Body jumps to the loop header (unless it diverged)
        graph->getNode(body->getNext()).addNext(headerMergeNode);
        graph->getNode(headerMergeNode).addPrev(body->getNext());
        headerBlock = &graph->getBasicBlock(headerStartBlockId);
        headerBlock->addPrevBlock(body->getSelfId());
    }

    // Tie up any continue statements
    assert(!pendingContinueBlocks.empty());
    headerBlock = &graph->getBasicBlock(headerStartBlockId);
    for (uint16_t continueBlockId : pendingContinueBlocks.back()) {
        BasicBlock* continueBlock = &graph->getBasicBlock(continueBlockId);
        graph->getNode(continueBlock->getNext()).addNext(headerMergeNode);
        graph->getNode(headerMergeNode).addPrev(continueBlock->getNext());
        headerBlock->addPrevBlock(continueBlockId);
    }
    pendingContinueBlocks.pop_back();
    headerBlock->seal();

    // Create block for loop exit
    BasicBlock *exitBlock = &addBasicBlock({headerBlock->getSelfId()}, block->getScope());
    exitBlock->addNode({GraphNodeType::IfFalse}, headerLoopNode);
    exitBlock->seal();

    // Tie up any break statements and merge
    vector<uint16_t> mergePrevs;
    vector<uint16_t> mergePrevBlocks;
    assert(!pendingBreakBlocks.empty());
    for (uint16_t breakBlockId : pendingBreakBlocks.back()) {
        BasicBlock* breakBlock = &graph->getBasicBlock(breakBlockId);
        mergePrevBlocks.push_back(breakBlockId);
        mergePrevs.push_back(breakBlock->getNext());
    }
    pendingBreakBlocks.pop_back();

    // Create merge if we need to
    if (mergePrevs.empty()) {
        return exitBlock;
    } else {
        mergePrevs.push_back(exitBlock->getNext());
        mergePrevBlocks.push_back(exitBlock->getSelfId());
        BasicBlock *mergeBlock = &addBasicBlock({mergePrevBlocks}, block->getScope());
        mergeBlock->addNode({GraphNodeType::Merge}, mergePrevs);
        mergeBlock->seal();
        return mergeBlock;
    }
}

BasicBlock *GraphBuilder::processForOfStatement(BasicBlock *block, ForOfStatement &node)
{
    // Add new block for loop header
    BasicBlock *headerBlock = &addBasicBlock({block->getSelfId()}, block->getScope().scopeForChildNode(&node));
    uint16_t headerStartBlockId = headerBlock->getSelfId();
    uint16_t headerMergeNode = headerBlock->addNode({GraphNodeType::Merge}, block->getNext());
    headerBlock = processAstNode(headerBlock, *node.getRight());
    uint16_t headerLoopNode = headerBlock->addNode({GraphNodeType::ForOfLoop, headerBlock->getNewest()}, headerBlock->getNext());

    // Add body block
    pendingBreakBlocks.emplace_back();
    pendingContinueBlocks.emplace_back();
    BasicBlock *body = &addBasicBlock({headerBlock->getSelfId()}, headerBlock->getScope().scopeForChildNode(node.getBody()));
    body->seal();
    body->addNode({GraphNodeType::IfTrue}, headerLoopNode);

    // Process declaration of loop var
    // TODO: Make a generic function that takes the value assigned to the pattern and declares the right variables recursively by extracting from the value
    if (node.getLeft()->getType() == AstNodeType::Identifier) {
        auto id = (Identifier*)node.getLeft();
        writeVariableById(*body, *id, headerLoopNode);
    } else if (node.getLeft()->getType() == AstNodeType::VariableDeclaration) {
        auto decls = ((VariableDeclaration*)node.getLeft())->getDeclarators();
        assert(decls.size() == 1);
        auto declId = decls[0]->getId();
        if (declId->getType() == AstNodeType::Identifier) {
            body->writeVariable((Identifier*)declId, headerLoopNode);
        } else if (declId->getType() == AstNodeType::ArrayPattern) {
            const auto& elems = ((ArrayPattern*)declId)->getElements();
            for (auto elem : elems) {
                if (elem->getType() != AstNodeType::Identifier) {
                    trace(node, "GraphBuilder cannot handle for-of with "s+elem->getTypeName()+" in left-hand side ArrayPattern");
                    throw runtime_error("GraphBuilder cannot handle for-of with "s+elem->getTypeName()+" in left-hand side ArrayPattern");
                }
                // TODO: Somehow generate a LoadNamedProperty, or a LoadIndexedProperty, or something similar that keeps track of which index we're extracting...
                // The problem being that we can't currently represent a literal for the index because we can only use literals that come from the AST
                // (Maybe for numeric literals we could repurpose input 0 to be the actual value... since it's a literal we always know it anyways! Kind of a huge hack tho)
                // (Or just add a value double to the graph node already, it's only 64bits and saves us a lot of trouble, then we can add a NumericLiteral type)
                body->addNode({GraphNodeType::LoadProperty, headerLoopNode, elem}, body->getNext());
                body->writeVariable((Identifier*)elem, body->getNewest());
            }
        } else {
            trace(node, "GraphBuilder cannot handle for-of with "s+declId->getTypeName()+" left-hand side");
            throw runtime_error("GraphBuilder cannot handle for-of with "s+declId->getTypeName()+" left-hand side");
        }
    }

    // Fill body
    body = processAstNode(body, *node.getBody());
    if (!body->isFilled()) {
        // Body jumps to the loop header (unless it diverged)
        graph->getNode(body->getNext()).addNext(headerMergeNode);
        graph->getNode(headerMergeNode).addPrev(body->getNext());
        headerBlock = &graph->getBasicBlock(headerStartBlockId);
        headerBlock->addPrevBlock(body->getSelfId());
    }

    // Tie up any continue statements
    assert(!pendingContinueBlocks.empty());
    headerBlock = &graph->getBasicBlock(headerStartBlockId);
    for (uint16_t continueBlockId : pendingContinueBlocks.back()) {
        BasicBlock* continueBlock = &graph->getBasicBlock(continueBlockId);
        graph->getNode(continueBlock->getNext()).addNext(headerMergeNode);
        graph->getNode(headerMergeNode).addPrev(continueBlock->getNext());
        headerBlock->addPrevBlock(continueBlockId);
    }
    pendingContinueBlocks.pop_back();
    headerBlock->seal();

    // Create block for loop exit
    BasicBlock *exitBlock = &addBasicBlock({headerBlock->getSelfId()}, block->getScope());
    exitBlock->addNode({GraphNodeType::IfFalse}, headerLoopNode);
    exitBlock->seal();

    // Tie up any break statements and merge
    vector<uint16_t> mergePrevs;
    vector<uint16_t> mergePrevBlocks;
    assert(!pendingBreakBlocks.empty());
    for (uint16_t breakBlockId : pendingBreakBlocks.back()) {
        BasicBlock* breakBlock = &graph->getBasicBlock(breakBlockId);
        mergePrevBlocks.push_back(breakBlockId);
        mergePrevs.push_back(breakBlock->getNext());
    }
    pendingBreakBlocks.pop_back();

    // Create merge if we need to
    if (mergePrevs.empty()) {
        return exitBlock;
    } else {
        mergePrevs.push_back(exitBlock->getNext());
        mergePrevBlocks.push_back(exitBlock->getSelfId());
        BasicBlock *mergeBlock = &addBasicBlock({mergePrevBlocks}, block->getScope());
        mergeBlock->addNode({GraphNodeType::Merge}, mergePrevs);
        mergeBlock->seal();
        return mergeBlock;
    }
}

BasicBlock *GraphBuilder::processConditionalExpression(BasicBlock *block, ConditionalExpression &node)
{
    block = processAstNode(block, *node.getTest());
    block->addNode({GraphNodeType::If, block->getNewest()}, block->getNext());
    vector<uint16_t> mergePrevs;

    // Add then block & node
    BasicBlock *consequent = &addBasicBlock({block->getSelfId()}, block->getScope().scopeForChildNode(node.getConsequent()));
    consequent->seal();
    consequent->addNode({GraphNodeType::IfTrue}, block->getNext());
    consequent = processAstNode(consequent, *node.getConsequent());
    uint16_t consequentId = consequent->getSelfId();
    assert(!consequent->isFilled());
    mergePrevs.push_back(consequent->getNext());
    uint16_t consequentNewest = consequent->getNewest();

    // Add alternate block (and node, if any)
    AstNode* alternateNode = node.getAlternate();
    BasicBlock *alternate = &addBasicBlock({block->getSelfId()}, block->getScope().scopeForChildNode(alternateNode));
    alternate->seal();
    alternate->addNode({GraphNodeType::IfFalse}, block->getNext());
    assert(alternateNode);
    alternate = processAstNode(alternate, *alternateNode);
    uint16_t alternateId = alternate->getSelfId();
    assert(!alternate->isFilled());
    mergePrevs.push_back(alternate->getNext());
    uint16_t alternateNewest = alternate->getNewest();

    // Create block for merge and add merge node + phi
    BasicBlock *mergeBlock = &addBasicBlock({consequentId, alternateId}, block->getScope());
    mergeBlock->addNode({GraphNodeType::Merge}, mergePrevs);
    mergeBlock->seal();

    // Add our phi with the newest on each side as inputs
    graph->getNode(mergeBlock->addPhi({consequentNewest, alternateNewest}));
    return mergeBlock;
}

BasicBlock *GraphBuilder::processTryStatement(BasicBlock *block, TryStatement &node)
{
    BasicBlock *tryBlock = &addBasicBlock({block->getSelfId()}, block->getScope().scopeForChildNode(node.getBlock()));
    uint16_t tryNodeId = tryBlock->addNode({GraphNodeType::Try, &node}, block->getNext());
    tryBlock->seal();

    BasicBlock *catchBlock = nullptr;
    BasicBlock *mergeBlock = nullptr;
    vector<uint16_t> mergePrevs;
    vector<uint16_t> mergePrevBlocks;

    if (auto handler = node.getHandler()) {
        if (node.getFinalizer()) { // Both cactch and finally
            trace(node, "Cannot handle finally clauses");
            throw runtime_error("Cannot handle finally clauses");
        }

        // Prepare the catch header, so that throwing in the try{} find our handler
        catchBlock = &addBasicBlock({block->getSelfId()}, block->getScope().scopeForChildNode(handler)); // Because we see variables declared in prev block, not e.g. in the try
        catchBlock->seal();
        catchStack.push_back(catchBlock->addNode({GraphNodeType::CatchException, tryNodeId}, true));

        auto catchParam = handler->getParam();
        if (catchParam->getType() == AstNodeType::Identifier) {
            catchBlock->writeVariable((Identifier*)catchParam, catchStack.back());
        } else {
            fatal("Cannot handle "s+catchParam->getTypeName()+" catch clause parameter");
        }

        // Process the try block
        tryBlock = processAstNode(&graph->getBasicBlock(tryBlock->getSelfId()), *node.getBlock());
        catchStack.pop_back();
        auto& lastTryNode = graph->getNode(tryBlock->getNext());
        if (!tryBlock->isFilled() && lastTryNode.getType() != GraphNodeType::Return && lastTryNode.getType() != GraphNodeType::Throw) {
            mergePrevs.push_back(tryBlock->getNext());
            mergePrevBlocks.push_back(tryBlock->getSelfId());
        }

        // Process the catch
        catchBlock = processAstNode(&graph->getBasicBlock(catchBlock->getSelfId()), *handler->getBody());
        auto& lastCatchNode = graph->getNode(catchBlock->getNext());
        if (!catchBlock->isFilled() && lastCatchNode.getType() != GraphNodeType::Return && lastCatchNode.getType() != GraphNodeType::Throw) {
            mergePrevs.push_back(catchBlock->getNext());
            mergePrevBlocks.push_back(catchBlock->getSelfId());
        }
    } else { // We have just a finally
        trace(node, "Cannot handle finally clauses");
        throw runtime_error("Cannot handle finally clauses");
    }

    mergeBlock = &addBasicBlock(move(mergePrevBlocks), block->getScope());
    if (!mergePrevs.empty()) {
        mergeBlock->addNode({GraphNodeType::Merge}, mergePrevs);
        mergeBlock->seal();
    } else {
        mergeBlock->seal();
        mergeBlock->setFilled();
    }

    return mergeBlock;
}

BasicBlock *GraphBuilder::processThrowStatement(BasicBlock *block, ThrowStatement &node)
{
    block = processAstNode(block, *node.getArgument());
    block->addNode({GraphNodeType::PrepareException, block->getNewest(), &node}, block->getNext());
    uint16_t prepareNode = block->getNext();

    if (catchStack.empty()) {
        block->addNode({GraphNodeType::Throw, &node}, block->getNext());
    } else {
        graph->getNode(prepareNode).addNext(catchStack.back());
        auto& catchNode = graph->getNode(catchStack.back());
        catchNode.addPrev(prepareNode);
    }

    block->setFilled();
    return block;
}

BasicBlock *GraphBuilder::processIdentifierNode(BasicBlock *block, Identifier &node)
{
    const auto& resolvedIds = parentModule.getResolvedLocalIdentifiers();
    auto declarationIt = resolvedIds.find(&node);
    Identifier* declarationIdentifier = declarationIt != resolvedIds.end() ? declarationIt->second : nullptr; // May be null if we couldn't resolve it, that's ok
    if (declarationIdentifier)
        trace(*declarationIdentifier, "Read "+node.getName());
    else
        trace(node, "Unknown declaration identifier Read "+node.getName());
    if (uint16_t* existingVar = block->readVariable(declarationIdentifier)) {
        block->setNewest(*existingVar);
    } else if (isChildOf(declarationIdentifier, *fun.getBody())) {
        // The variable isn't local to this basic block, we need to run global value numbering
        auto value = block->readNonlocalVariable(*declarationIdentifier);
        if (value == 0)
            trace(fun, "About to fail graphbuilder assert for function:\n"+fun.getSourceString());
        assert(value != 0);
        block->setNewest(value);
    } else {
        bool isParam = false;
        for (auto param : fun.getParams()) {
            if (isChildOf(declarationIdentifier, *param)) {
                block->addNode({GraphNodeType::LoadParameter, declarationIdentifier}, block->getNext());
                isParam = true;
                break;
            }
        }
        if (!isParam)
            block->addNode({GraphNodeType::LoadValue, &node}, block->getNext());
    }
    return block;
}

BasicBlock *GraphBuilder::processAssignmentExprNode(BasicBlock *block, AssignmentExpression &node)
{
    AstNode* left = node.getLeft();
    trace(*left, "Write assign");

    if (left->getType() == AstNodeType::Identifier) {
        if (node.getOperator() == AssignmentExpression::Operator::Equal) {
            block = processAstNode(block, *node.getRight());
        } else {
            block = processAstNode(block, *left);
            uint16_t leftValue = block->getNewest();
            block = processAstNode(block, *node.getRight());
            block->addNode({GraphNodeType::BinaryOperator, {leftValue, block->getNewest()}, &node});
        }
        writeVariableById(*block, *(Identifier*)left, block->getNewest());
    } else if (left->getType() == AstNodeType::MemberExpression) {
        auto leftExpr = (MemberExpression*)left;
        block = processAstNode(block, *leftExpr->getObject());
        uint16_t object = block->getNewest();

        auto propNode = leftExpr->getProperty();
        if (leftExpr->isComputed()) {
            block = processAstNode(block, *propNode);
            uint16_t prop = block->getNewest();

            block = processAstNode(block, *node.getRight());
            uint16_t value = block->getNewest();

            block->addNode({GraphNodeType::StoreProperty, {object, prop, value}, propNode}, block->getNext());
        } else {
            assert(propNode->getType() == AstNodeType::Identifier);

            block = processAstNode(block, *node.getRight());
            uint16_t value = block->getNewest();

            block->addNode({GraphNodeType::StoreNamedProperty, {object, value}, propNode}, block->getNext());
        }
    } else {
        trace(node, "GraphBuilder cannot handle complex assignment!");
        throw runtime_error("GraphBuilder cannot handle complex assignment!");
    }
    return block;
}

BasicBlock *GraphBuilder::processCallExprNode(BasicBlock *block, CallExpression &node)
{
    block = processAstNode(block, *node.getCallee());
    uint16_t calleeNode = block->getNewest();

    vector<uint16_t> inputs = {calleeNode};
    auto args = node.getArguments();
    for (AstNode* arg : args) {
        block = processAstNode(block, *arg);
        inputs.push_back(block->getNewest());
    }


    if (node.getType() == AstNodeType::NewExpression)
        block->addNode({GraphNodeType::NewCall, move(inputs), &node}, block->getNext());
    else
        block->addNode({GraphNodeType::Call, move(inputs), &node}, block->getNext());
    return block;
}

BasicBlock *GraphBuilder::processArrayExprNode(BasicBlock *block, ArrayExpression &node)
{
    vector<uint16_t> elemNodes;
    for (auto elem : node.getElements()) {
        if (!elem) {
            block->setNewest(graph->getUndefinedNode());
        } else {
            block = processAstNode(block, *elem);
        }
        elemNodes.push_back(block->getNewest());
    }
    block->addNode({GraphNodeType::ArrayLiteral, move(elemNodes), &node});

    return block;
}

BasicBlock *GraphBuilder::processObjectExprNode(BasicBlock *block, ObjectExpression &node)
{
    vector<uint16_t> elemNodes;
    const auto& props = node.getProperties();
    for (auto& prop : props) {
        block = processAstNode(block, *prop);
        elemNodes.push_back(block->getNewest());
    }

    block->addNode({GraphNodeType::ObjectLiteral, move(elemNodes), &node});
    return block;
}

BasicBlock *GraphBuilder::processObjectPropNode(BasicBlock *block, ObjectProperty &node)
{
    if (node.isComputed()) {
        block = processAstNode(block, *node.getKey());
        uint16_t keyNode = block->getNewest();
        block = processAstNode(block, *node.getValue());
        block->addNode({GraphNodeType::ObjectProperty, {block->getNewest(), keyNode}, &node});
    } else {
        assert(node.getKey()->getType() == AstNodeType::Identifier
               || node.getKey()->getType() == AstNodeType::StringLiteral
               || node.getKey()->getType() == AstNodeType::NumericLiteral);
        block = processAstNode(block, *node.getValue());
        block->addNode({GraphNodeType::ObjectProperty, block->getNewest(), &node});
    }

    return block;
}

BasicBlock *GraphBuilder::processSpreadElemNode(BasicBlock *block, SpreadElement &node)
{
    block = processAstNode(block, *node.getArgument());
    block->addNode({GraphNodeType::Spread, block->getNewest(), &node});
    return block;
}

BasicBlock *GraphBuilder::processTemplateLiteralNode(BasicBlock *block, TemplateLiteral &node)
{
    vector<uint16_t> inputs;
    for (auto expr : node.getExpressions()) {
        block = processAstNode(block, *expr);
        inputs.push_back(block->getNewest());
    }
    block->addNode({GraphNodeType::TemplateLiteral, move(inputs), &node});
    return block;
}

BasicBlock* GraphBuilder::processVariableDeclarationNode(BasicBlock* block, VariableDeclaration &node)
{
    // We don't actually add a node, just record a value write in the basic block
    auto& decls = ((VariableDeclaration&)node).getDeclarators();
    for (VariableDeclarator* decl : decls) {
        AstNode* init = decl->getInit();
        AstNode* idNode = decl->getId();
        if (init) {
            block = processAstNode(block, *init);
            if (idNode->getType() == AstNodeType::Identifier) {
                block->writeVariable((Identifier*)idNode, block->getNewest());
            } else if (idNode->getType() == AstNodeType::ObjectPattern) {
                block = processObjectPatternNode(block, (ObjectPattern&)*idNode, block->getNewest());
            } else {
                trace(node, "GraphBuilder cannot handle declaration with "s+idNode->getTypeName()+" left-hand side");
                throw runtime_error("GraphBuilder cannot handle declaration with "s+idNode->getTypeName()+" left-hand side");
            }
        } else {
            block->writeVariable((Identifier*)idNode, graph->getUndefinedNode());
        }
    }
    return block;
}

BasicBlock *GraphBuilder::processObjectPatternNode(BasicBlock *block, ObjectPattern &node, uint16_t object)
{
    for (AstNode* prop : node.getProperties()) {
        if (prop->getType() == AstNodeType::ObjectProperty) {
            auto* objProp = (ObjectProperty*)prop;
            if (objProp->isComputed()) {
                block = processAstNode(block, *objProp->getKey());
                block->addNode({GraphNodeType::LoadProperty, {object, block->getNewest()}, prop}, block->getNext());
            } else {
                assert(objProp->getKey()->getType() == AstNodeType::Identifier);
                block->addNode({GraphNodeType::LoadNamedProperty, object, objProp->getKey()}, block->getNext());
            }
            uint16_t loadedKey = block->getNewest();

            AstNode* value = objProp->getValue();
            if (value->getType() == AstNodeType::Identifier) {
                trace(*objProp->getValue(), "Write object pattern prop "+((Identifier*)value)->getName());
                block->writeVariable(objProp->getValue(), loadedKey);
            } else if (value->getType() == AstNodeType::ObjectPattern) {
                return processObjectPatternNode(block, (ObjectPattern&)*value, loadedKey);
            } else {
                fatal("Cannot process "s+value->getTypeName()+" for value node in object pattern");
            }
        } else {
            trace(node, "GraphBuilder cannot handle "s+prop->getTypeName()+" object patterns");
            throw runtime_error("GraphBuilder cannot handle "s+prop->getTypeName()+" object patterns");
        }
    }

    return block;
}

BasicBlock *GraphBuilder::processMemberExprNode(BasicBlock *block, MemberExpression &node)
{
    block = processAstNode(block, *node.getObject());
    uint16_t object = block->getNewest();

    auto prop = node.getProperty();
    if (node.isComputed()) {
        block = processAstNode(block, *prop);
        block->addNode({GraphNodeType::LoadProperty, {object, block->getNewest()}, prop}, block->getNext());
    } else {
        assert(prop->getType() == AstNodeType::Identifier);
        block->addNode({GraphNodeType::LoadNamedProperty, object, prop}, block->getNext());
    }

    return block;
}

BasicBlock* GraphBuilder::processFunctionNode(BasicBlock* block, Function &node)
{
    // Hoisting may have already declared this function (if it was visible from a block, and not an expression)
    if (auto existingVar = block->readVariable(node.getId())) {
        block->setNewest(*existingVar);
    } else {
        block->addNode({GraphNodeType::Function, &node}, block->getNext());
        if (auto* id = node.getId()) {
            assert(node.getType() == AstNodeType::FunctionExpression || node.getType() == AstNodeType::FunctionDeclaration);
            block->writeVariable(id, block->getNewest());
        }
    }
    return block;
}

BasicBlock *GraphBuilder::processUnaryExprNode(BasicBlock *block, UnaryExpression &node)
{
    block = processAstNode(block, *node.getArgument());
    block->addNode({GraphNodeType::UnaryOperator, block->getNewest(), &node});
    return block;
}

BasicBlock* GraphBuilder::processBinaryExprNode(BasicBlock* block, BinaryExpression &node)
{
    block = processAstNode(block, *node.getLeft());
    uint16_t left = block->getNewest();
    block = processAstNode(block, *node.getRight());
    uint16_t right = block->getNewest();
    block->addNode({GraphNodeType::BinaryOperator, {left, right}, &node});
    return block;
}

BasicBlock *GraphBuilder::processUpdateExprNode(BasicBlock *block, UpdateExpression &node)
{
    uint16_t argValue;
    AstNode* arg = node.getArgument();
    trace(*arg, "Write update expr");

    // NOTE: We currentyl ignore prefix/postfix distinction, because it doesn't affect types (and the fix isn't obvious!)
    // TODO: Somehow fix non-prefix UpdatExpr so that a same-expr read returns prev value, but other statements see updated value
    // It's annoying because currently the node result is expected to be the new value of the variable...

    if (arg->getType() == AstNodeType::Identifier) {
        block = processAstNode(block, *arg);
        argValue = block->getNewest();
        block->addNode({GraphNodeType::UnaryOperator, argValue, &node});
        writeVariableById(*block, *(Identifier*)arg, block->getNewest());
    } else if (arg->getType() == AstNodeType::MemberExpression) {
        auto leftExpr = (MemberExpression*)arg;
        block = processAstNode(block, *leftExpr->getObject());
        uint16_t object = block->getNewest();

        auto propNode = leftExpr->getProperty();
        if (leftExpr->isComputed()) {
            block = processAstNode(block, *propNode);
            argValue = block->getNewest();

            block->addNode({GraphNodeType::UnaryOperator, argValue, &node});
            uint16_t value = block->getNewest();

            block->addNode({GraphNodeType::StoreProperty, {object, argValue, value}, propNode}, block->getNext());
        } else {
            assert(propNode->getType() == AstNodeType::Identifier);
            block->addNode({GraphNodeType::LoadNamedProperty, object, propNode}, block->getNext());
            argValue = block->getNewest();

            block->addNode({GraphNodeType::UnaryOperator, argValue, &node});
            uint16_t value = block->getNewest();

            block->addNode({GraphNodeType::StoreNamedProperty, {object, value}, propNode}, block->getNext());
        }
    } else {
        trace(node, "GraphBuilder cannot handle complex lhs in update expressions!");
        throw runtime_error("GraphBuilder cannot handle complex lhs in update expressions!");
    }
    return block;
}

BasicBlock *GraphBuilder::processLogicalExprNode(BasicBlock *block, LogicalExpression &node)
{
    block = processAstNode(block, *node.getLeft());
    uint16_t left = block->getNewest();
    block = processAstNode(block, *node.getRight());
    uint16_t right = block->getNewest();
    block->addNode({GraphNodeType::BinaryOperator, {left, right}, &node});
    return block;
}

BasicBlock *GraphBuilder::processTypeCastExpr(BasicBlock *block, TypeCastExpression &node)
{
    block = processAstNode(block, *node.getExpression());
    block->addNode({GraphNodeType::TypeCast, block->getNewest(), &node});
    return block;
}

BasicBlock *GraphBuilder::processSwitchStatement(BasicBlock *block, SwitchStatement &node)
{
    BasicBlock *tryBlock = &addBasicBlock({block->getSelfId()}, block->getScope().scopeForChildNode(&node));
    block->setFilled();
    tryBlock->setNext(block->getNext());
    tryBlock->seal();
    tryBlock = processAstNode(tryBlock, *node.getDiscriminant());
    uint16_t discriminantNode = tryBlock->getNewest();

    tryBlock->addNode({GraphNodeType::Switch, discriminantNode}, tryBlock->getNext());
    tryBlock->setFilled();
    uint16_t switchNodeId = tryBlock->getNext();
    uint16_t prevBlockId = tryBlock->getSelfId();
    vector<uint16_t> mergePrevs;
    vector<uint16_t> mergePrevBlocks;

    if (node.getCases().empty()) {
        BasicBlock* exitBlock = &addBasicBlock({prevBlockId}, block->getScope());
        exitBlock->seal();
        return exitBlock;
    }

    pendingBreakBlocks.emplace_back();
    uint16_t prevCaseBlockId = 0;
    for (SwitchCase* caseNode : node.getCases()) {
        BasicBlock *caseBlock = &addBasicBlock({prevBlockId}, tryBlock->getScope().scopeForChildNode(&node));
        caseBlock->setNext(switchNodeId);
        if (BasicBlock *prevCaseBlock = prevCaseBlockId ? &graph->getBasicBlock(prevCaseBlockId) : nullptr) {
            if (!prevCaseBlock->isFilled()) {
                caseBlock->addPrevBlock(prevCaseBlock->getSelfId());
                vector caseMergePrevs{switchNodeId, prevCaseBlock->getNext()};
                caseBlock->addNode({GraphNodeType::Merge}, caseMergePrevs);
            }
        }
        caseBlock->seal();

        if (caseNode->getTest()) {
            caseBlock = processAstNode(caseBlock, *caseNode->getTest());
            caseBlock->addNode({GraphNodeType::Case, caseBlock->getNewest()}, caseBlock->getNext());
        } else {
            caseBlock->addNode({GraphNodeType::Case}, caseBlock->getNext());
        }

        for (auto caseConsequent : caseNode->getConsequents())
            caseBlock = processAstNode(caseBlock, *caseConsequent);

        prevCaseBlockId = caseBlock->getSelfId();
    }

    if (BasicBlock *lastCaseBlock = prevCaseBlockId ? &graph->getBasicBlock(prevCaseBlockId) : nullptr; lastCaseBlock && !lastCaseBlock->isFilled()) {
        mergePrevBlocks.push_back(prevCaseBlockId);
        mergePrevs.push_back(lastCaseBlock->getNext());
    }

    // Tie up any break statements
    assert(!pendingBreakBlocks.empty());
    for (uint16_t breakBlockId : pendingBreakBlocks.back()) {
        BasicBlock* breakBlock = &graph->getBasicBlock(breakBlockId);
        mergePrevBlocks.push_back(breakBlockId);
        mergePrevs.push_back(breakBlock->getNext());
    }
    pendingBreakBlocks.pop_back();

    // Create block for merge and add merge node
    BasicBlock *mergeBlock = &addBasicBlock(move(mergePrevBlocks), block->getScope());
    mergeBlock->seal();
    if (!mergePrevs.empty()) {
        mergeBlock->addNode({GraphNodeType::Merge}, mergePrevs);
    } else {
        mergeBlock->setFilled();
    }

    return mergeBlock;
}

BasicBlock *GraphBuilder::processBreakStatement(BasicBlock *block, BreakStatement &node)
{
    if (node.getLabel()) {
        fatal("Break to lavel not supported by graphbuilder");
    }

    block->addNode({GraphNodeType::Break}, block->getNext());
    block->setFilled();

    if (pendingBreakBlocks.empty()) {
        error(node, "break statement outside of a loop, switch or labeled-block");
    } else {
        pendingBreakBlocks.back().push_back(block->getSelfId());
    }

    return block;
}

BasicBlock *GraphBuilder::processContinueStatement(BasicBlock *block, ContinueStatement &node)
{
    if (node.getLabel()) {
        fatal("Continue to lavel not supported by graphbuilder");
    }

    block->addNode({GraphNodeType::Continue}, block->getNext());
    block->setFilled();

    if (pendingContinueBlocks.empty()) {
        error(node, "Continue statement outside of a loop or labeled-block");
    } else {
        pendingContinueBlocks.back().push_back(block->getSelfId());
    }

    return block;
}
