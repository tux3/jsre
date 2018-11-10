#include "unused.hpp"
#include "module/module.hpp"
#include "utils/reporting.hpp"
#include "ast/ast.hpp"
#include "analyze/astqueries.hpp"
#include <string>

using namespace std;

static bool isNonlocalImportedIdentifier(Identifier& id)
{
    auto parent = id.getParent();
    if (parent->getType() != AstNodeType::ImportSpecifier)
        return false;
    auto spec = (ImportSpecifier*)parent;
    return spec->getImported() == &id;
}

static bool isIdentifierOfExportedDeclaration(Identifier& id)
{
    auto parent = id.getParent();
    switch (parent->getType()) {
    case AstNodeType::ExportSpecifier:
        return true;
    case AstNodeType::ClassDeclaration:
        if (((ClassDeclaration*)parent)->getId() != &id)
            return false;
        break;
    case AstNodeType::FunctionDeclaration:
        if (((FunctionDeclaration*)parent)->getId() != &id)
            return false;
        break;
    case AstNodeType::VariableDeclarator:
        if (((VariableDeclarator*)parent)->getId() != &id)
            return false;
        parent = parent->getParent();
        break;
    case AstNodeType::TypeAlias:
        if (((TypeAlias*)parent)->getId() != &id)
            return false;
        break;
    default:
        return false;
    }

    parent = parent->getParent();
    return parent->getType() == AstNodeType::ExportNamedDeclaration
            || parent->getType() == AstNodeType::ExportDefaultDeclaration;
}

static bool isFunctionNamedForStacktraces(Identifier& id)
{
    if (!isFunctionNode(*id.getParent()))
        return false;
    auto& parent = (Function&)*id.getParent();

    if (parent.getId() != &id)
        return false;

    return parent.getParent()->getType() == AstNodeType::ObjectProperty;
}

void findUnusedLocalDeclarations(Module &module)
{
    const auto& declarationsXRefs = module.getLocalXRefs();
    for (const auto& elem : declarationsXRefs) {
        if (elem.second.size() > 1)
            continue;
        auto& identifier = *elem.first;
        if (isNonlocalImportedIdentifier(identifier)
            || identifier.getParent()->getType() == AstNodeType::CatchClause // Catch clauses are syntactically required to take an arg
            || isIdentifierOfExportedDeclaration(identifier))
            continue;

        // Properties and methods of objects/classes are accessed through member expressions, which we can't resolve yet
        // We don't know their usage at all, so we can't way whether any of those are unused.
        if (isUnscopedPropertyOrMethodIdentifier(identifier))
            continue;

        // Non-type identifiers in type declarations, like paraneter names, are not unused since they are unscope
        if (isUnscopedTypeIdentifier(identifier))
            continue;

        // When writing {foo: function foo() {}}, the function name is unused, but without it the function looks anonymous in stacktraces
        if (isFunctionNamedForStacktraces(identifier))
            continue;

        // There's no way to remove unused params in a function expression, so no warning, but it's convention to start their name with a '_'
        if (isFunctionalExpressionArgumentIdentifier(identifier)) {
            if (identifier.getName()[0] != '_')
                suggest(identifier, "Rename unused parameter "+identifier.getName()+" to _"+identifier.getName());
            continue;
        } else if (identifier.getParent()->getType() == AstNodeType::ImportSpecifier) {
            warn(identifier, "Unused import of "s+identifier.getName());
        } else if (isFunctionParameterIdentifier(identifier)) {
            warn(identifier, "Unused parameter "s+identifier.getName());
        } else {
            warn(identifier, "Unused declaration of identifier "s+identifier.getName());
        }
    }
}
