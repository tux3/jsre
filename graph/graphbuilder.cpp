#include "graphbuilder.hpp"
#include "ast/walk.hpp"
#include "analyze/astqueries.hpp"
#include "reporting.hpp"
#include "module/module.hpp"
#include <cassert>

using namespace std;

GraphBuilder::GraphBuilder(Function &fun)
    : fun{fun}, parentModule{fun.getParentModule()}
{
}

std::unique_ptr<Graph> GraphBuilder::buildFromAst()
{
    graph = make_unique<Graph>();

    processAstNode(&graph->getBasicBlock(0), fun);

    vector<uint16_t> leaves;
    for (uint16_t i=0; i<graph->size(); ++i) {
        GraphNode& node = graph->getNode(i);
        if (node.prevCount() > 0 && node.nextCount() == 0)
            leaves.push_back(i);
    }
    graph->addNode({GraphNodeType::End}, leaves);

    return std::move(graph);
}

BasicBlock* GraphBuilder::processAstNode(BasicBlock* block, AstNode &node)
{
    walkAst(node, [&](AstNode& node) {
        switch (node.getType()) {
        case AstNodeType::BlockStatement:
            for (AstNode* child : node.getChildren())
                if (child)
                    block = processAstNode(block, *child);
            break;
        case AstNodeType::FunctionDeclaration:
        case AstNodeType::FunctionExpression:
        case AstNodeType::ArrowFunctionExpression:
        case AstNodeType::ClassMethod:
        case AstNodeType::ClassPrivateMethod:
        case AstNodeType::ObjectMethod:
            block = processFunctionNode(block, (Function&)node);
            break;
        case AstNodeType::ReturnStatement:
            block = processAstNode(block, *((ReturnStatement&)node).getArgument());
            block->addNode({GraphNodeType::Return, block->getNewest()}, block->getNext());
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
        case AstNodeType::MemberExpression:
            block = processMemberExprNode(block, (MemberExpression&)node);
            break;
        case AstNodeType::AssignmentExpression:
            block = processAssignmentExprNode(block, (AssignmentExpression&)node);
            break;
        case AstNodeType::NullLiteral:
        case AstNodeType::NumericLiteral:
        case AstNodeType::BooleanLiteral:
        case AstNodeType::StringLiteral:
        case AstNodeType::RegExpLiteral:
            block->addNode({GraphNodeType::Literal, &node});
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
        case AstNodeType::TemplateLiteral:
            // TODO: Handle TemplateLiterals, they're not like other literals since they depend on variables/expressions!
            fatal(node, "GraphBuilder cannot handle "s+node.getTypeName()+" AST nodes!");
        default:
            fatal(node, "GraphBuilder cannot handle "s+node.getTypeName()+" AST nodes!");
        }
    }, [&](AstNode& node) {
        return WalkDecision::WalkOver;
    });

    return block;
}

BasicBlock *GraphBuilder::processIfStatement(BasicBlock *block, IfStatement &node)
{
    block = processAstNode(block, *node.getTest());
    block->addNode({GraphNodeType::If, block->getNewest()}, block->getNext());
    uint16_t prevNodeId = block->getNext();
    uint16_t prevBlockId = block->getSelfId();
    vector<uint16_t> mergePrevs;

    // Add then block & node
    BasicBlock *consequent = &graph->addBasicBlock({prevBlockId});
    uint16_t consequentId = consequent->getSelfId();
    consequent->seal();
    consequent->addNode({GraphNodeType::IfTrue}, prevNodeId);
    consequent = processAstNode(consequent, *node.getConsequent());
    mergePrevs.push_back(consequent->getNext());

    // Add alternate block (and node, if any)
    AstNode* alternateNode = node.getAlternate();
    BasicBlock *alternate = &graph->addBasicBlock({prevBlockId});
    uint16_t alternateId = alternate->getSelfId();
    alternate->seal();
    alternate->addNode({GraphNodeType::IfFalse}, prevNodeId);
    if (alternateNode)
        alternate = processAstNode(alternate, *alternateNode);
    mergePrevs.push_back(alternate->getNext());

    // Create block for merge and add merge node
    BasicBlock *mergeBlock = &graph->addBasicBlock({consequentId, alternateId});
    mergeBlock->addNode({GraphNodeType::Merge}, mergePrevs);
    mergeBlock->seal();

    return mergeBlock;
}

uint16_t GraphBuilder::readNonlocalVariable(Identifier &declIdentifier, BasicBlock *block)
{
    // TODO:
    // If a block is not sealed, we have to add an operandless/incomplete phi in that block, and we'll complete it later when the block is sealed
    // Otherwise if we only have one parent block, we recurse into it
    // And if we have multiple parent blocks, we put a phi in the current block and recursively look for the last defition in all parents for that phi
    // After all that is done, we can simplify some phis

    if (uint16_t* existingVar = block->readVariable(&declIdentifier))
        return *existingVar;

    if (!block->isSealed()) {
        fatal(declIdentifier, "GraphBuilder GVN for non-sealed blocks not implemented yet!");
    } else if (block->getPrevs().size() == 1) {
        return readNonlocalVariable(declIdentifier, &graph->getBasicBlock(block->getPrevs()[0]));
    } else {
        uint16_t phiId = block->addPhi();
        block->writeVariable(&declIdentifier, phiId);
        return completePhi(declIdentifier, block, phiId);
    }

    fatal(declIdentifier, "GraphBuilder GVN not implemented yet!");
}

uint16_t GraphBuilder::completePhi(Identifier &declIdentifier, BasicBlock *block, uint16_t phi)
{
    auto prevs = block->getPrevs();
    for (auto prevId : prevs) {
        warn(to_string(prevId));
        BasicBlock& prevBlock = graph->getBasicBlock(prevId);
        if (uint16_t* existingVar = prevBlock.readVariable(&declIdentifier)) {
            graph->getNode(phi).addInput(*existingVar);
        } else {
            graph->getNode(phi).addInput(readNonlocalVariable(declIdentifier, &prevBlock));
        }
    }

    // TODO: We may be able to remove some trivial phis, that would change the id we return, and we have to change all existing users (block defs and graph node uses)
    return phi;
}

BasicBlock *GraphBuilder::processIdentifierNode(BasicBlock *block, Identifier &node)
{
    Identifier* declarationIdentifier = parentModule.getResolvedLocalIdentifiers()[&node]; // May be null if we couldn't resolve it, that's ok
    if (declarationIdentifier)
        warn(*declarationIdentifier, "Read "+node.getName());
    else
        warn(node, "Unknown declaration identifier Read "+node.getName());
    if (uint16_t* existingVar = block->readVariable(declarationIdentifier)) {
        // What if we declared a variable without initializer and read it's value? "let x; return x;".
        // The value's undefined, but we don't have a way to represent that! Instead we use node 0 (Start) as input to represent undefined or no such input
        block->setNewest(*existingVar);
    } else if (isChildOf(declarationIdentifier, *fun.getBody())) {
        // The variable isn't local to this basic block, we need to run global value numbering
        block->setNewest(readNonlocalVariable(*declarationIdentifier, block));
    } else {
        block->addNode({GraphNodeType::LoadValue, &node}, block->getNext());
    }
    return block;
}

BasicBlock *GraphBuilder::processAssignmentExprNode(BasicBlock *block, AssignmentExpression &node)
{
    AstNode* left = node.getLeft();
    warn(*left, "Write assign");

    if (left->getType() == AstNodeType::Identifier) {
        block = processAstNode(block, *node.getRight());
        uint16_t value = block->getNewest();

        Identifier* declId = parentModule.getResolvedLocalIdentifiers()[(Identifier*)left];
        if (isChildOf(declId, *fun.getBody()))
            block->writeVariable(declId, value);
        else
            block->addNode({GraphNodeType::StoreValue, value, left}, block->getNext());
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

            block->addNode({GraphNodeType::StoreProperty, {object, prop, value}}, block->getNext());
        } else {
            assert(prop->getType() == AstNodeType::Identifier);

            block = processAstNode(block, *node.getRight());
            uint16_t value = block->getNewest();

            block->addNode({GraphNodeType::StoreNamedProperty, {object, value}, propNode}, block->getNext());
        }
    } else {
        fatal(node, "GraphBuilder cannot handle complex assignment!");
    }
    return block;
}

BasicBlock* GraphBuilder::processVariableDeclarationNode(BasicBlock* block, VariableDeclaration &node)
{
    // We don't actually add a node, just record a value write in the basic block
    auto& decls = ((VariableDeclaration&)node).getDeclarators();
    for (VariableDeclarator* decl : decls) {
        AstNode* init = decl->getInit();
        if (init) {
            warn(*decl->getId(), "Write val");
            block = processAstNode(block, *init);
            block->writeVariable(decl->getId(), block->getNewest());
        } else {
            warn(*decl->getId(), "Write undefined");
            block->writeVariable(decl->getId(), 0); // Input node 0 means undefined / no input
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
        block->addNode({GraphNodeType::LoadProperty, {object, block->getNewest()}}, block->getNext());
    } else {
        assert(prop->getType() == AstNodeType::Identifier);
        block->addNode({GraphNodeType::LoadNamedProperty, object, prop}, block->getNext());
    }

    return block;
}

BasicBlock* GraphBuilder::processFunctionNode(BasicBlock* block, Function &node)
{
    AstNode* body = node.getBody();
    block = processAstNode(block, *body);
    if (body->getType() != AstNodeType::BlockStatement)
        block->addNode({GraphNodeType::Return, block->getNewest()}, block->getNext());

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
