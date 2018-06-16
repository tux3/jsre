#ifndef IDENTRESOLUTION_HPP
#define IDENTRESOLUTION_HPP

#include <unordered_map>
#include <v8.h>

class AstNode;
class AstRoot;
class Identifier;
class ImportSpecifier;

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
IdentifierResolutionResult resolveModuleIdentifiers(v8::Local<v8::Context> context, AstRoot& ast);

/**
 * When necessary inserts a global object with value undefined to serve as a definition
 * This prevents ReferenceErrors when evaluating a module in this context
 *
 * unresolvedTopLevelIdentifiers should be the result returned by resolveModuleIdentifiers
 */
void defineMissingGlobalIdentifiers(v8::Local<v8::Context> context, const std::vector<std::string>& missingGlobalIdentifiers);

/**
 * Finds the declaration of the identifier local to the imported module and imported by that specifier.
 * This does not try to return an ExportSpecifier, but the declaration of the identifier under its original name.
 * Does not handle ImportNamespaceSpecifiers, since those arguably don't refer to a particular Ast node
 * If the imported module is re-exporting an imported identifier, we do not recursively resolve it, we return the local declaration.
 * In particular in the case the imported identifier comes from an ExportAllDeclaration, this is the node we return.
 *
 * Return nullptr if this import specifier refers to a native module, or if the declaration couldn't be found.
 */
AstNode* resolveImportedIdentifierDeclaration(AstNode& importSpec);

/**
 * Finds the original (non-import) declaration for this (potentially imported) identifier, in whatever module originally declared it.
 * This does not try to return an ExportSpecifier, but the declaration of the identifier under its original name.
 * Note that if the identifier is declared as a variable, we do return the variable declarator node, and not its initializer (if any)!
 *
 * Return nullptr if this import specifier refers to a native module, or if the declaration couldn't be found.
 */
AstNode* resolveIdentifierDeclaration(Identifier& identifier);

#endif // IDENTRESOLUTION_HPP
