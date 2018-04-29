#ifndef ASTQUERIES_HPP
#define ASTQUERIES_HPP

class AstNode;
class Identifier;

// True is this identifier introduces a property or method of an object or class, but is not part of any scope (accessed through member expressions)
bool isUnscopedPropertyOrMethodIdentifier(Identifier& node);

// True if the identifier is a non-type identifier is a type declaration, and so is not part of any scope (purely informational identifier)
bool isUnscopedTypeIdentifier(Identifier& node);

// True if the identifier is for a parameter of an ArrowFunctionExpression or FunctionExpression
bool isFunctionalExpressionArgumentIdentifier(Identifier& node);

// True if the identifier is for a parameter of a Function node
bool isFunctionParameterIdentifier(Identifier& node);

// True if the node is a Function&
bool isFunctionNode(AstNode& node);

#endif // ASTQUERIES_HPP
