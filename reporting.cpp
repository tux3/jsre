#include "reporting.hpp"
#include "ast/ast.hpp"
#include "module/module.hpp"
#include <iostream>
#include <cstdlib>
#include <filesystem>

using namespace std;

static bool debugEnabled = false;
static bool suggestEnabled = false;

static ReportingStats globalStats;

void setDebug(bool enable)
{
    debugEnabled = enable;
}

void setSuggest(bool enable)
{
    suggestEnabled = enable;
}

static void printLocation(AstNode& node)
{
    Module& mod = node.getParentModule();
    string relativePath = filesystem::relative(mod.getPath());
    auto loc = node.getLocation().start;
    cout << relativePath << ':' << loc.line << ':' <<loc.column << ": ";
}

void trace(const string &msg)
{
    if (!debugEnabled)
        return;
    cout << "debug: " << msg << endl;
    globalStats.traces++;
}

void trace(AstNode &node, const string &msg)
{
    if (!debugEnabled)
        return;
    printLocation(node);
    trace(msg);
}

void suggest(const string &msg)
{
    globalStats.suggestions++;
    if (!suggestEnabled)
        return;
    cout << "suggest: " << msg << endl;
}

void suggest(AstNode &node, const string &msg)
{
    if (suggestEnabled)
        printLocation(node);
    suggest(msg);
}

void warn(const std::string &msg)
{
    globalStats.warnings++;
    cout << "warning: " << msg << endl;
}

void warn(AstNode &node, const string &msg)
{
    printLocation(node);
    warn(msg);
}

void error(const string &msg)
{
    globalStats.errors++;
    cout << "error: " << msg << endl;
}

void error(AstNode &node, const string &msg)
{
    printLocation(node);
    error(msg);
}

void fatal(const string &msg)
{
    cout << "Error: " << msg << endl;
    abort();
}

void fatal(AstNode &node, const string &msg)
{
    printLocation(node);
    fatal(msg);
}

const ReportingStats &getReportingStatistics()
{
    return globalStats;
}
