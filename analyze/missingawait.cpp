#include "missingawait.hpp"
#include "module/module.hpp"
#include "reporting.hpp"
#include "ast/ast.hpp"
#include "ast/walk.hpp"
#include "analyze/astqueries.hpp"
#include "analyze/identresolution.hpp"
#include "queries/dataflow.hpp"
#include "queries/types.hpp"

using namespace std;

void findMissingAwaits(Module &module)
{
    AstRoot& ast = module.getAst();

    walkAst(ast, [&](AstNode& node){
        if (node.getType() != AstNodeType::CallExpression)
            return;
        auto& call = (CallExpression&)node;
        if (call.getParent()->getType() == AstNodeType::AwaitExpression)
            return;

        AstNode* calleeNode = call.getCallee();
        if (calleeNode->getType() != AstNodeType::Identifier)
            return;
        auto callee = (Identifier*)calleeNode;
        AstNode* resolvedCallee = resolveIdentifierDeclaration(*callee);
        if (!resolvedCallee)
            return;
        // The callee could be a (function/callable) parameter declared in the parent function, we don't handle that at all currently
        // TODO: Use the query system to directly ask for the type of the result of the call expression (and whether it's a Promise)
        if (!isFunctionNode(*resolvedCallee))
            return;
        if (returnsPromiseType((Function&)*resolvedCallee) != Tribool::Yep)
            return;

        if (isReturnedValue(call) == Tribool::Yep) {
            AstNode* parent = &call;
            while (parent && !isFunctionNode(*parent))
                parent = parent->getParent();
            if (returnsPromiseType((Function&)*parent) == Tribool::Yep)
                return;

            suggest(*calleeNode, "Function returns a promise, not a value. Make the function async, or add a type annotation."s);
        } else {
            // TODO: If the promise is assigned to a variable, track the use of that variable and warn if we treat it like a T instead of a Promise<T>
            // TODO: If we call .then() or .catch() on the promise at any point, don't say anything
            suggest(*calleeNode, "Possible missing await"s);
        }
    });

}
