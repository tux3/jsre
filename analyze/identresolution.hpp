#ifndef IDENTRESOLUTION_HPP
#define IDENTRESOLUTION_HPP

#include <unordered_map>
#include <v8.h>

class AstNode;
class AstRoot;
class Identifier;
class ImportSpecifier;
class MemberExpression;
class ThisExpression;

struct IdentifierResolutionResult
{
    std::unordered_map<Identifier*, Identifier*> resolvedIdentifiers;
    std::vector<std::string> missingGlobalIdentifiers;
};

/**
 * Tries to find the local declaration for every identifier in the AST
 *
 * Returns top-level identifiers that does not have a declaration.
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
 * Note that the import specifier may actually be an ExportSpecifier if the ExportNamedDeclaration has a source (importing-export)
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
 *
 * WARNING: The identifier is not necessarily the name of the returned declaration, it could also be another name introduce by the declaration!
 *          In particular for Function* nodes, we consider that the parameters are not declared by the parent functions, but declare themselves
 *          Otherwise callers couldn't tell whether the identifier refers to the function or its argument: `function test(test) { return test }`
 */
AstNode* resolveIdentifierDeclaration(Identifier& identifier);

/**
 * Tries to resolve the static target of a member expression
 * The primary use case is for resolving member function calls.
 *
 * NOTE: This is entirely static, so it will only work in simple cases, and only when the target is an AstNode* (as opposed to a runtime value)
 *
 * Returns nullptr if there isn't a static target, or if we couldn't find it.
 */
AstNode* resolveMemberExpression(MemberExpression& expr);

/**
 * Tries to find the static target for a ThisExpression
 * Since it is static, it won't work inside normal functions, only arrow functions and class methods
 *
 * Returns nullptr if there isn't a static target, or if we couldn't find it.
 */
AstNode* resolveThisExpression(ThisExpression& thisExpression);

/**
 * Tries to find the static target for the value a ThisExpression would have inside the lexically scoping node
 * Since it is static, it won't work on normal functions, only arrow functions and class methods
 *
 * Returns nullptr if there isn't a static target, or if we couldn't find it.
 */
AstNode* resolveThisValue(AstNode& lexicalScope);

#endif // IDENTRESOLUTION_HPP
