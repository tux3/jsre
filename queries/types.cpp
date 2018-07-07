#include "types.hpp"
#include "ast/ast.hpp"

Tribool returnsPromiseType(Function &fun)
{
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
