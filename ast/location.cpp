#include "location.hpp"
#include <cassert>

AstSourcePosition::AstSourcePosition(unsigned offset, unsigned line, unsigned column)
    : offset{offset}, line{line}, column{column}
{
}

bool AstSourcePosition::operator>(const AstSourcePosition &other)
{
    return offset > other.offset;
}

bool AstSourcePosition::operator<(const AstSourcePosition &other)
{
    return offset < other.offset;
}

bool AstSourcePosition::operator>=(const AstSourcePosition &other)
{
    return offset >= other.offset;
}

bool AstSourcePosition::operator<=(const AstSourcePosition &other)
{
    return offset <= other.offset;
}

AstSourceSpan::AstSourceSpan(AstSourcePosition start, AstSourcePosition end)
    : start{start}, end{end}
{
    assert(end >= start);
}
