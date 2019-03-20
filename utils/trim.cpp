#include "trim.hpp"
#include <algorithm>
#include <cctype>
#include <locale>

// These functions merrily stolen from: https://stackoverflow.com/a/217605/1401962

// trim from start (in place)
std::string ltrim(std::string s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    return s;
}

// trim from end (in place)
std::string rtrim(std::string s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}

// trim from both ends (in place)
std::string trim(std::string s)
{
    return ltrim(rtrim(s));
}
