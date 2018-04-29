#include "flow.hpp"
#include "ast/ast.hpp"
#include "ast/walk.hpp"
#include "reporting.hpp"
#include "transform/blank.hpp"
#include <cctype>

using namespace std;

std::string stripFlowTypes(const std::string &source, AstNode &ast)
{
    std::string transformed = source;

    auto isTypeAnnotation = [](AstNode& node) {
        return node.getType() == AstNodeType::TypeAnnotation;
    };
    auto isTypeAlias = [](AstNode& node) {
        return node.getType() == AstNodeType::TypeAlias;
    };
    auto isTypeExportDeclaration = [](AstNode& node) {
        return node.getType() == AstNodeType::ExportNamedDeclaration
            && ((ExportNamedDeclaration&)node).getKind() == ExportNamedDeclaration::Kind::Type;
    };
    auto isTypeImportDeclaration = [](AstNode& node) {
        return node.getType() == AstNodeType::ImportDeclaration
                && ((ImportDeclaration&)node).getKind() == ImportDeclaration::Kind::Type;
    };
    auto isTypeParameterDeclaration = [](AstNode& node) {
        return node.getType() == AstNodeType::TypeParameterDeclaration;
    };
    auto isTypeImportSpecifier = [](AstNode& node) {
        return node.getType() == AstNodeType::ImportSpecifier && ((ImportSpecifier&)node).isTypeImport();
    };
    auto isOptionalIdentifier = [](AstNode& node) {
        return node.getType() == AstNodeType::Identifier && ((Identifier&)node).isOptional();
    };
    auto isClassWithImplements = [](AstNode& node) {
        return (node.getType() == AstNodeType::ClassDeclaration
                || node.getType() == AstNodeType::ClassExpression)
                && !((Class&)node).getImplements().empty();
    };

    walkAst(ast, [&](AstNode& node){
        // Optional identifiers have a trailing '?' before the type annotation
        if (isOptionalIdentifier(node)) {
            blankNext(transformed, node.getLocation().start.offset, '?');
        } else if (isTypeImportSpecifier(node)) {
            blankNodeFromSource(transformed, node);
            blankNextComma(transformed, node); // The ',' following the specifier needs to be removed, if any
        } else if (isClassWithImplements(node)) {
            size_t startPos = ((ClassDeclaration&)node).getId()->getLocation().end.offset;
            blankUntil(transformed, startPos, '{');
        } else if (isTypeAnnotation(node)
                   || isTypeAlias(node)
                   || isTypeExportDeclaration(node)
                   || isTypeImportDeclaration(node)
                   || isTypeParameterDeclaration(node)) {
            blankNodeFromSource(transformed, node);
        }
    });

    return transformed;
}
