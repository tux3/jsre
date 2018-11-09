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
    uint16_t getUndefinedNode();
    BasicBlock* processArgument(BasicBlock* block, AstNode& node);
    void hoistFunctionNode(BasicBlock* block, Function &node);
    void hoistVariableDeclarationNode(BasicBlock* block, VariableDeclaration &node);
    // Returns the basic block where next nodes should be added
    BasicBlock* processAstNode(BasicBlock* block, AstNode& node);
    BasicBlock* processFunctionNode(BasicBlock* block, Function &node);
    BasicBlock* processUnaryExprNode(BasicBlock* block, UnaryExpression &node);
    BasicBlock* processBinaryExprNode(BasicBlock* block, BinaryExpression &node);
    BasicBlock* processUpdateExprNode(BasicBlock* block, UpdateExpression &node);
    BasicBlock* processLogicalExprNode(BasicBlock* block, LogicalExpression &node);
    BasicBlock* processMemberExprNode(BasicBlock* block, MemberExpression &node);
    BasicBlock* processAssignmentExprNode(BasicBlock* block, AssignmentExpression &node);
    BasicBlock* processCallExprNode(BasicBlock* block, CallExpression &node);
    BasicBlock* processArrayExprNode(BasicBlock* block, ArrayExpression &node);
    BasicBlock* processObjectExprNode(BasicBlock* block, ObjectExpression &node);
    BasicBlock* processObjectPropNode(BasicBlock* block, ObjectProperty &node);
    BasicBlock* processSpreadElemNode(BasicBlock* block, SpreadElement &node);
    BasicBlock* processTemplateLiteralNode(BasicBlock* block, TemplateLiteral &node);
    BasicBlock* processVariableDeclarationNode(BasicBlock* block, VariableDeclaration &node);
    BasicBlock* processIdentifierNode(BasicBlock* block, Identifier &node);
    BasicBlock* processObjectPatternNode(BasicBlock* block, ObjectPattern &node, uint16_t object);
    BasicBlock* processIfStatement(BasicBlock* block, IfStatement& node);
    BasicBlock* processWhileStatement(BasicBlock* block, WhileStatement& node);
    BasicBlock* processDoWhileStatement(BasicBlock* block, DoWhileStatement& node);
    BasicBlock* processForStatement(BasicBlock* block, ForStatement& node);
    BasicBlock* processForOfStatement(BasicBlock* block, ForOfStatement& node);
    BasicBlock* processConditionalExpression(BasicBlock* block, ConditionalExpression& node);
    BasicBlock* processThrowStatement(BasicBlock* block, ThrowStatement& node);
    BasicBlock* processTryStatement(BasicBlock* block, TryStatement& node);
    BasicBlock* processTypeCastExpr(BasicBlock* block, TypeCastExpression& node);
    BasicBlock* processSwitchStatement(BasicBlock* block, SwitchStatement& node);
    BasicBlock* processBreakStatement(BasicBlock* block, BreakStatement& node);
    BasicBlock* processContinueStatement(BasicBlock* block, ContinueStatement& node);

private:
    std::unique_ptr<Graph> graph;
    std::vector<uint16_t> catchStack; // Nodes able to catch an exception thrown in the current block (last has priority)
    std::vector<std::vector<uint16_t>> pendingBreakBlocks; // Blocks of break nodes that haven't been tied up into a merge by their parent yet, one vector per parent scope.
    std::vector<std::vector<uint16_t>> pendingContinueBlocks; // Blocks of continue nodes that haven't been tied up into a merge by their parent yet, one vector per parent scope.
    Function& fun;
    Module& parentModule;
    uint16_t undefinedNode;
};

#endif // GRAPHBUILDER_HPP
