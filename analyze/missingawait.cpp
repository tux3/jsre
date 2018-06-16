#include "missingawait.hpp"
#include "module/module.hpp"
#include "reporting.hpp"
#include "ast/ast.hpp"
#include "ast/walk.hpp"
#include "analyze/astqueries.hpp"
#include "analyze/identresolution.hpp"
#include "queries/dataflow.hpp"

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
        if (!isFunctionNode(*resolvedCallee))
            return;
        auto resolvedFunction = (Function*)resolvedCallee;
        if (!resolvedFunction->isAsync())
            return;

        if (isReturnedValue(call) == Tribool::Yep) {
            AstNode* parent = &call;
            while (parent && !isFunctionNode(*parent))
                parent = parent->getParent();
            auto parentFunction = (Function*)parent;
            if (parentFunction->isAsync())
                return;
            // TODO: If the function has a flow annotation for a Promise<> return type, we should return
        }

        // TODO: If the promise is assigned to a variable, track the use of that variable and warn if we treat it like a T instead of a Promise<T>

        suggest(*calleeNode, "Possible missing await"s);
    });

}
