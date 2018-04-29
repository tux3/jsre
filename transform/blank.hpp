#ifndef BLANK_HPP
#define BLANK_HPP

#include <string>

class AstNode;

// Functions in this file preserve the AST offset locations
// (counted in UTF8 characters), but not the line numbers

// Replaces an entire AST node with spaces in the source code.
void blankNodeFromSource(std::string& source, AstNode& node);

// If the next non-whitespace character is a ',' we blank it
void blankNextComma(std::string& source, AstNode& node);

// If the next non-whitespace character is a ',' we blank it
void blankNextComma(std::string& source, size_t position);

// Replaces the entire range with spaces
void blankRange(std::string& source, size_t position, size_t count);

// Replaces the next occurence of this byte with a space
void blankNext(std::string& source, size_t start, char byte);

// Replaces characters with a space until this byte (not included)
void blankUntil(std::string& source, size_t start, char byte);

#endif // BLANK_HPP
