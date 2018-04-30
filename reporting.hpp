#ifndef REPORTING_HPP
#define REPORTING_HPP

#include <string>

class AstNode;

// If left to false, trace messages will not be shown
void setDebug(bool enable);
// If left to false, suggest messages will not be shown
void setSuggest(bool enable);

void trace(const std::string& msg); //< Debug information.
void trace(AstNode& node, const std::string& msg); //< Debug information.
void suggest(const std::string& msg); //< Annoys you about minor or possible problems.
void suggest(AstNode& node, const std::string& msg); //< Annoys you about minor or possible problems.
void warn(const std::string& msg); //< Reports a real problem with your code.
void warn(AstNode& node, const std::string& msg); //< Reports a real problem with your code.
void error(const std::string& msg); //< Reports a bug in your code.
void error(AstNode& node, const std::string& msg); //< Reports a bug in your code.
[[noreturn]]
void fatal(const std::string& msg); //< Reports a fatal error. This will exit!
[[noreturn]]
void fatal(AstNode& node, const std::string& msg); //< Reports a fatal error. This will exit!

#endif // REPORTING_HPP
