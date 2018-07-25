#include "types.hpp"
#include "ast/ast.hpp"

#include "reporting.hpp"
#include "graph/graphbuilder.hpp"
#include "graph/dot.hpp"

Tribool returnsPromiseType(Function &fun)
{
    warn(fun, "### GENERATING GRAPH FOR FUNCTION "+(fun.getId()?fun.getId()->getName():"<anonymous>"));
    GraphBuilder graphBuilder(fun);
    auto graph = graphBuilder.buildFromAst();
    warn("Graph data:\n"+graphToDOT(*graph));

    if (fun.isAsync())
        return Tribool::Yep;

    if (AstNode* returnType = fun.getReturnTypeAnnotation()) {
        if (returnType->getType() == AstNodeType::GenericTypeAnnotation
            && ((GenericTypeAnnotation*)returnType)->getId()->getName() == "Promise") {
            return Tribool::Yep;
        } else {
            return Tribool::Nope;
        }
    }

    return Tribool::Maybe;
}
