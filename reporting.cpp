#include "reporting.hpp"
#include "ast/ast.hpp"
#include "module/module.hpp"
#include <iostream>
#include <cstdlib>

using namespace std;

static void printLocation(AstNode& node)
{
    Module& mod = node.getParentModule();
    cout << mod.getPath();
    auto loc = node.getLocation().start;
    cout << ':' << loc.line << ':' <<loc.column << ": ";
}

void trace(const string &msg)
{
    cout << "debug: " << msg << endl;
}

void trace(AstNode &node, const string &msg)
{
    printLocation(node);
    trace(msg);
}

void suggest(const string &msg)
{
    cout << "suggest: " << msg << endl;
}

void suggest(AstNode &node, const string &msg)
{
    printLocation(node);
    suggest(msg);
}

void warn(const std::string &msg)
{
    cout << "warning: "<<msg<<endl;
}

void warn(AstNode &node, const string &msg)
{
    printLocation(node);
    warn(msg);
}

void error(const string &msg)
{
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
    exit(EXIT_FAILURE);
}

void fatal(AstNode &node, const string &msg)
{
    printLocation(node);
    fatal(msg);
}
