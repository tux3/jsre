#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>

struct AstSourcePosition
{
    AstSourcePosition(unsigned offset, unsigned line, unsigned column);

    /// Note that those are character counts, ant NOT byte counts!
    unsigned offset;
    unsigned line;
    unsigned column;

    bool operator>(const AstSourcePosition& other);
    bool operator<(const AstSourcePosition& other);
    bool operator>=(const AstSourcePosition& other);
    bool operator<=(const AstSourcePosition& other);
};

struct AstSourceSpan
{
    AstSourceSpan(AstSourcePosition start, AstSourcePosition end);
    std::string toString(const std::string& source); // NOTE: This is slow, due to UTF8 processing

    AstSourcePosition start, end;
};

#endif // LOCATION_HPP
