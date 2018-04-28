#ifndef REPORTING_HPP
#define REPORTING_HPP

#include <string>

void trace(const std::string& msg); //< Debug information.
void suggest(const std::string& msg); //< Annoys you about minor or possible problems.
void warn(const std::string& msg); //< Reports a real problem with your code.
void error(const std::string& msg); //< Reports a bug in your code.
[[noreturn]]
void fatal(const std::string& msg); //< Reports a fatal error. This will exit!

#endif // REPORTING_HPP
