#ifndef REPORTING_HPP
#define REPORTING_HPP

#include <string>
#include <atomic>

class AstNode;

struct ReportingStats {
    std::atomic_int traces = 0;
    std::atomic_int suggestions = 0;
    std::atomic_int warnings = 0;
    std::atomic_int errors = 0;
};

// If left to false, trace messages will not be shown
void setDebug(bool enable);
// If left to false, suggest messages will not be shown
void setSuggest(bool enable);

// Returns the current statistics on the number of reports since the start
const ReportingStats& getReportingStatistics();
void resetReportingStatistics();

void trace(const std::string& msg); //< Debug information.
void trace(const AstNode &node, const std::string& msg); //< Debug information.
void suggest(const std::string& msg); //< Annoys you about minor or possible problems.
void suggest(const AstNode& node, const std::string& msg); //< Annoys you about minor or possible problems.
void warn(const std::string& msg); //< Reports a real problem with your code.
void warn(const AstNode& node, const std::string& msg); //< Reports a real problem with your code.
void error(const std::string& msg); //< Reports a bug in your code.
void error(const AstNode& node, const std::string& msg); //< Reports a bug in your code.
[[noreturn]]
void fatal(const std::string& msg); //< Reports a fatal error. This will exit!
[[noreturn]]
void fatal(const AstNode& node, const std::string& msg); //< Reports a fatal error. This will exit!

#endif // REPORTING_HPP
