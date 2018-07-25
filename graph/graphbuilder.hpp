#ifndef GRAPHBUILDER_HPP
#define GRAPHBUILDER_HPP

#include "graph/graph.hpp"
#include "ast/ast.hpp"
#include <memory>

class Module;
class Function;
class BasicBlock;
class GraphBuilder
{
public:
    GraphBuilder(Function& fun);
    std::unique_ptr<Graph> buildFromAst();

private:
    // Returns the basic block where next nodes should be added
    BasicBlock* processAstNode(BasicBlock* block, AstNode& node);
    BasicBlock* processFunctionNode(BasicBlock* block, Function &node);
    BasicBlock* processUnaryExprNode(BasicBlock* block, UnaryExpression &node);
    BasicBlock* processBinaryExprNode(BasicBlock* block, BinaryExpression &node);
    BasicBlock* processMemberExprNode(BasicBlock* block, MemberExpression &node);
    BasicBlock* processAssignmentExprNode(BasicBlock* block, AssignmentExpression &node);
    BasicBlock* processVariableDeclarationNode(BasicBlock* block, VariableDeclaration &node);
    BasicBlock* processIdentifierNode(BasicBlock* block, Identifier &node);
    BasicBlock* processIfStatement(BasicBlock* block, IfStatement& node);

    uint16_t readNonlocalVariable(Identifier& declIdentifier, BasicBlock* block);
    uint16_t completePhi(Identifier& declIdentifier, BasicBlock *block, uint16_t phi);

private:
    std::unique_ptr<Graph> graph;
    Function& fun;
    Module& parentModule;
};

#endif // GRAPHBUILDER_HPP
