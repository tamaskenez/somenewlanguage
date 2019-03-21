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

// To store an UTF8 char sequence. Gives no guarantees about validity.
// xs[0] is always the first byte of the sequence.
// Unused upper bytes are ignored.
struct Utf8Char
{
    array<char8_t, 4> xs;

    Utf8Char() = default;

    // Constructor sets first byte, no check for ASCII validity (can be first byte of longer
    // sequence).
    explicit Utf8Char(char8_t c) { xs[0] = c; }

    // Compare to ASCII character.
    // c expected to be valid ASCII, that is c <= 0x7F.
    bool operator==(char c) const
    {
        assert(!(c & 0x80));
        return xs[0] == c;
    }
    bool is_utf8_bom() const { return xs[0] == 0xef && xs[1] == 0xbb && xs[2] == 0xbf; }

    // Return 0 if invalid.
    int size() const
    {
        if (is_ascii_utf8_byte(xs[0]))
            return 1;
        if (is_leading_byte_of_two_byte_utf8_seq(xs[0]))
            return 2;
        if (is_leading_byte_of_three_byte_utf8_seq(xs[0]))
            return 3;
        if (is_leading_byte_of_four_byte_utf8_seq(xs[0]))
            return 4;
        return 0;
    }
    maybe<char32_t> code_point() const;
};

string to_descriptive_string(Utf8Char c);

}  // namespace forrest
