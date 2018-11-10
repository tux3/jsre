#include "module/module.hpp"
#include "utils/reporting.hpp"
#include "ast/ast.hpp"
#include "ast/walk.hpp"
#include "analyze/astqueries.hpp"
#include "analyze/identresolution.hpp"
#include "queries/dataflow.hpp"
#include "queries/typeresolution.hpp"
#include "graph/graph.hpp"

using namespace std;

void missingAwaitFunctionPass(Module &module, Graph& graph)
{
    for (uint16_t i=0; i<graph.size(); ++i) {
        GraphNode& node = graph.getNode(i);
        if (node.getType() != GraphNodeType::Call)
            continue;

        TypeInfo type = resolveNodeType(graph, &node);
        if (type.getBaseType() != BaseType::Promise)
            continue;

        AstNode& callAstNode = *node.getAstReference();
        if (callAstNode.getParent()->getType() == AstNodeType::AwaitExpression)
            return;

        if (isReturnedValue(callAstNode) == Tribool::Yep) {
            AstNode* parent = &callAstNode;
            while (parent && !isFunctionNode(*parent))
                parent = parent->getParent();

            auto& parentFun = (Function&)*parent;
            if (parentFun.isAsync())
                return;
            if (AstNode* returnType = parentFun.getReturnTypeAnnotation()) {
                if (returnType->getType() == AstNodeType::GenericTypeAnnotation
                    && ((GenericTypeAnnotation*)returnType)->getId()->getName() == "Promise") {
                    return;
                }
            }

            suggest(callAstNode, "Function returns a promise, not a value. Mark the function async, or add a type annotation."s);
        } else {
            // TODO:Track the use of that variable and warn if we don't either call then/catch on it, or pass it to a function that expects a promise

            // If we immediately call .then() or .catch() on the result, nothing to report
            if (callAstNode.getParent()->getType() == AstNodeType::MemberExpression
                    && ((MemberExpression*)callAstNode.getParent())->getProperty()->getType() == AstNodeType::Identifier
                    && callAstNode.getParent()->getParent()->getType() == AstNodeType::CallExpression) {
                auto name = ((Identifier*)((MemberExpression*)callAstNode.getParent())->getProperty())->getName();
                if (name == "catch" || name == "then")
                    return;
            }
            warn(callAstNode, "Possible missing await"s);
        }
    }
}
