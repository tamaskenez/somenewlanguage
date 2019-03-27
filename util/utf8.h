#pragma once

#include <array>
#include <string>

#include "util/ensure_char8_t.h"
#include "util/maybe.h"

namespace forrest {

using std::array;
using std::string;

inline bool is_ascii_utf8_byte(char8_t c)
{
    return !(c & 0x80);
}
inline bool is_utf8_continuation_byte(char8_t c)
{
    return (c & 0xc0) == 0x80;
}
inline bool is_leading_byte_of_two_byte_utf8_seq(char8_t c)
{
    return (c & 0xe0) == 0xc0;
}
inline bool is_leading_byte_of_three_byte_utf8_seq(char8_t c)
{
    return (c & 0xf0) == 0xe0;
}
inline bool is_leading_byte_of_four_byte_utf8_seq(char8_t c)
{
    return (c & 0xf8) == 0xf0;
}

// To store a valid UTF8 char sequence, validity is only assert-ensured.
class Utf8Char
{
    array<char8_t, 4> xs;

public:
    Utf8Char() { xs.fill(0); }

    // Expects valid ASCII char (c <= 0x7f)
    explicit Utf8Char(char c)
    {
        assert(is_ascii_utf8_byte(c));
        xs[0] = c;
    }

    // Expects valid, 1..4 long utf8 byte sequence
    explicit Utf8Char(array<char8_t, 4> a) : xs(a) { assert(size() != 0); }
    void operator=(char c)
    {
        assert(is_ascii_utf8_byte(c));
        xs[0] = c;
    }

    // Compare to ASCII character.
    // c expected to be valid ASCII, that is c <= 0x7F.
    bool operator==(char c) const
    {
        assert(is_ascii_utf8_byte(c));
        return xs[0] == c;
    }
    bool operator!=(char c) const
    {
        assert(is_ascii_utf8_byte(c));
        return xs[0] != c;
    }
    bool is_utf8_bom() const { return xs[0] == 0xef && xs[1] == 0xbb && xs[2] == 0xbf; }

    // Return 1..4
    // Return 0 if it happends to be invalid (NDEBUG only).
    int size() const;

    // Return nothing if it happens to be invalid (NDEBUG only).
    maybe<char32_t> code_point() const;
    auto begin() const { return xs.begin(); }
    auto end() const { return xs.begin() + size(); }
    char8_t front() const { return xs[0]; }
};

string to_descriptive_string(Utf8Char c);
string utf32_to_descriptive_string(char32_t c);
bool is_equal_u8string_asciicstr(const u8string& a, const char* b);

}  // namespace forrest
