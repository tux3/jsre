#ifndef IDENTRESOLUTION_HPP
#define IDENTRESOLUTION_HPP

#include <unordered_map>
#include <v8.h>

class AstRoot;
class Identifier;

struct IdentifierResolutionResult
{
    std::unordered_map<Identifier*, Identifier*> resolvedIdentifiers;
    std::vector<std::string> missingGlobalIdentifiers;
};

/**
 * Tries to find the local declaration for every identifier in the AST
 *
 * Warns about top-level identifiers that does not have a declaration, and returns them.
 * Those are the identifiers that would cause a ReferecenceError when evaluating or importing the module
 *
 * Returns a map of identifiers to their point of declaration in the AST.
 * Indentifiers for which a declaration wasn't found aren't included in the map.
 */
IdentifierResolutionResult resolveModuleIdentifiers(v8::Local<v8::Context> context, AstRoot* ast);

/**
 * When necessary inserts a global object with value undefined to serve as a definition
 * This prevents ReferenceErrors when evaluating a module in this context
 *
 * unresolvedTopLevelIdentifiers should be the result returned by resolveModuleIdentifiers
 */
void defineMissingGlobalIdentifiers(v8::Local<v8::Context> context, const std::vector<std::string>& missingGlobalIdentifiers);

#endif // IDENTRESOLUTION_HPP
