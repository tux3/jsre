#include "typerefinement.hpp"
#include "analyze/typecheck.hpp"
#include "ast/ast.hpp"
#include "graph/graph.hpp"
#include "module/module.hpp"
#include "queries/types.hpp"
#include "queries/typeresolution.hpp"
#include "queries/maybe.hpp"
#include "utils/reporting.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <cassert>

using namespace std;

static void refineByTruthiness(TypeInfo& type, bool truthy)
{
    if (type.getBaseType() != BaseType::Sum)
        return;
    auto sumElems = type.getExtra<SumTypeInfo>()->elements;
    if (truthy) {
        sumElems.erase(remove_if(sumElems.begin(), sumElems.end(), [](const TypeInfo& elem) {
            return elem.getBaseType() == BaseType::Null || elem.getBaseType() == BaseType::Undefined;
        }));
        if (sumElems.size() > 1)
            type = TypeInfo::makeSum(move(sumElems));
        else
            type = sumElems[0];
    }
}

static unordered_map<GraphNode*, Tribool> inferRefinementsFromNode(Graph& graph, GraphNode* node, bool condIsTrue)
{
    unordered_map<GraphNode*, Tribool> truthinessMap;

    // We care about two categories of nodes:
    // - Boolean operators that combine to form the condition
    // - Nodes like function calls, phis or arguments whose type is opaque and can be refined
    if (node->getType() == GraphNodeType::Call || node->getType() == GraphNodeType::Phi) {
        truthinessMap.insert({node, condIsTrue ? Tribool::Yep : Tribool::Nope});
    } else if (node->getType() == GraphNodeType::UnaryOperator) {
        auto op = ((UnaryExpression*)node->getAstReference())->getOperator();
        if (op == UnaryExpression::Operator::LogicalNot) {
            truthinessMap = inferRefinementsFromNode(graph, &graph.getNode(node->getInput(0)), condIsTrue);
            for (auto& pair : truthinessMap)
                pair.second = !pair.second;
        }
    } else if (node->getType() == GraphNodeType::BinaryOperator && node->getAstReference()->getType() == AstNodeType::LogicalExpression) {
        auto op = ((LogicalExpression*)node->getAstReference())->getOperator();
        if ((op == LogicalExpression::Operator::And && condIsTrue)
            || (op == LogicalExpression::Operator::Or && !condIsTrue)) {
            auto left = inferRefinementsFromNode(graph, &graph.getNode(node->getInput(0)), condIsTrue);
            auto right = inferRefinementsFromNode(graph, &graph.getNode(node->getInput(1)), condIsTrue);
            for (auto& pair : left)
                truthinessMap[pair.first] = pair.second;
            for (auto& pair : right)
                if (pair.second != Tribool::Maybe) // We happily assume both sides don't contradict each other like `if (a && !a)`
                    truthinessMap[pair.first] = pair.second;
        }
    }

    return truthinessMap;
}

// When condIsTrue we deduce refinements for the "then" branch, when false for the "else" branch. branchNode can be If/Loop.
static void inferRefinementsFromIf(Graph& graph, ScopedTypes& scope, GraphNode* branchNode, bool condIsTrue)
{
    if (!branchNode->inputCount()) // Infinite for loops have no condition
        return;

    GraphNode* condNode = &graph.getNode(branchNode->getInput(0));
    auto truthinessMap = inferRefinementsFromNode(graph, condNode, condIsTrue);

    for (const auto& elem : truthinessMap) {
        if (elem.second == Tribool::Maybe)
            continue;
        auto it = scope.types.find(elem.first);
        if (it == scope.types.end())
            it = scope.types.insert({elem.first, resolveNodeType(graph, elem.first)}).first;
        refineByTruthiness(it->second, elem.second == Tribool::Yep ? true : false);
    }
}

void refineTypes(Graph& graph, ScopedTypes& scope, GraphNode* node)
{
    if (node->getType() == GraphNodeType::IfTrue) {
        inferRefinementsFromIf(graph, scope, &graph.getNode(node->getPrev(0)), true);
    } else if (node->getType() == GraphNodeType::IfFalse) {
        inferRefinementsFromIf(graph, scope, &graph.getNode(node->getPrev(0)), false);
    }
}
