#ifndef ASTQUERIES_HPP
#define ASTQUERIES_HPP

class Identifier;

// True is this identifier introduces a property or method of an object or class, but is not part of any scope (accessed through member expressions)
bool isUnscopedPropertyOrMethodIdentifier(Identifier& node);

// True if the identifier is for a parameter of an ArrowFunctionExpression or FunctionExpression
bool isFunctionalExpressionArgumentIdentifier(Identifier& node);

#endif // ASTQUERIES_HPP
