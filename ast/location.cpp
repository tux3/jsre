#include "location.hpp"
#include "utf8/utf8.h"
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

std::string AstSourceSpan::toString(const std::string &source)
{
    const char* beg_ptr = source.data();
    utf8::unchecked::advance(beg_ptr, start.offset);
    const char* end_ptr = beg_ptr;
    utf8::unchecked::advance(end_ptr, end.offset-start.offset);

    return std::string(beg_ptr, end_ptr);
}
