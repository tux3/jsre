#include "reporting.hpp"
#include <iostream>
#include <cstdlib>

using namespace std;

void trace(const string &msg)
{
    cout << "debug: " << msg << endl;
}

void suggest(const string &msg)
{
    cout << "suggest: " << msg << endl;
}

void warn(const std::string &msg)
{
    cout << "warning: "<<msg<<endl;
}

void error(const string &msg)
{
    cout << "error: " << msg << endl;
}

void fatal(const string &msg)
{
    cout << "Error: " << msg << endl;
    exit(EXIT_FAILURE);
}
