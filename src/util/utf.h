#pragma once

#include <array>
#include <string>

#include "util/maybe.h"

namespace forrest {

using std::array;
using std::string;

// TODO when this fails: add #defines to avoid redefining char8_t when it's supported in the
// language (C++20)
using char8_t = unsigned char;

static_assert(
    sizeof(char) == 1,
    "Think about this and probably replace char with uint8_t avoiding excessive memory usage.");
static_assert(
    sizeof(char8_t) == 1,
    "Think about this and probably replace char8_t with uint8_t avoiding excessive memory usage.");

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

// Because of the inconveniences (conversion) of using u8string for
// UTF-8 string, we use simply std::string for UTF-8 strings as the majority
// of strings are UTF-8 and std::strings has always been used that way.

// String that indicates (but does not guarantee) ASCII strings.
struct ascii_string
{
    string s;  // Intentionally public. Accept WG21/P0109R0.
    ascii_string() = default;
    ascii_string(const ascii_string&) = default;
    ascii_string& operator=(const ascii_string&) = default;
    ascii_string(ascii_string&&) = default;
    ascii_string& operator=(ascii_string&&) = default;
    /*
    ~ascii_string() { assert(is_valid()); }
    ascii_string(const char* s) : s(s) { assert(is_valid()); }
    ascii_string& operator(const char* s)
    {
        this.s = s;
        assert(is_valid());
        return *this;
    }
    */
    bool is_valid() const
    {
        for (auto c : s) {
            if (!is_ascii_utf8_byte(c))
                return false;
        }
        return true;
    }
};

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

}  // namespace forrest
