#ifndef LOCATION_HPP
#define LOCATION_HPP

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

    AstSourcePosition start, end;
};

#endif // LOCATION_HPP
