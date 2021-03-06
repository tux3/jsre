#include "blank.hpp"
#include "ast/ast.hpp"
#include "utf8/utf8.h"
#include <cassert>
#include <cstring>

void blankNodeFromSource(std::string &source, AstNode &node)
{
    auto loc = node.getLocation();
    assert(loc.start.offset <= loc.end.offset);
    assert(loc.end.offset < source.size());

    blankRange(source, loc.start.offset, loc.end.offset - loc.start.offset);
}

void blankNextComma(std::string &source, AstNode &node)
{
    blankNextComma(source, node.getLocation().end.offset);
}

void blankNextComma(std::string &source, size_t position)
{
    char* ptr = source.data();
    utf8::unchecked::advance(ptr, position);
    while (isspace(*ptr))
        ptr++;
    if (*ptr == ',')
        *ptr = ' ';
}

void blankRange(std::string &source, size_t position, size_t count)
{
    char* beg_ptr = source.data();
    utf8::unchecked::advance(beg_ptr, position);
    char* end_ptr = beg_ptr;
    utf8::unchecked::advance(end_ptr, count);

    size_t bytes_start_pos = static_cast<size_t>(beg_ptr-source.data());
    size_t bytes_count = static_cast<size_t>(end_ptr - beg_ptr);
    size_t extra_bytes = bytes_count - count;

    source.erase(bytes_start_pos, extra_bytes);
    memset(beg_ptr, ' ', count);
}

void blankNext(std::string &source, size_t start, char byte)
{
    char* ptr = source.data();
    utf8::unchecked::advance(ptr, start);

    while (*ptr != byte)
        utf8::unchecked::next(ptr);
    *ptr = ' ';
}

void blankUntil(std::string &source, size_t start, char byte)
{
    char* ptr = source.data();
    utf8::unchecked::advance(ptr, start);

    size_t char_count = 0;
    while (*ptr != byte) {
        utf8::unchecked::next(ptr);
        char_count++;
    }

    blankRange(source, start, char_count);
}
