#include "reporting.hpp"
#include <iostream>

using namespace std;

void warn(const std::string &msg)
{
    cout << "warning: "<<msg<<endl;
}

void trace(const string &msg)
{
    cout << "debug: " << msg << endl;
}
