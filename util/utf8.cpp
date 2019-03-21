#include "util/utf8.h"

#include "absl/strings/str_format.h"

namespace forrest {

using absl::StrFormat;

string to_descriptive_string(Utf8Char c)
{
    auto cp = c.code_point();
    if (cp <= 0x7f) {
        if (isgraph(c.xs[0]))
            return string(1, c.xs[0]);
        return StrFormat("U+00%02X", c.xs[0]);
    }
    return cp ? StrFormat("U+%04X", *cp) : string("<invalid UTF-8>");
}

maybe<char32_t> Utf8Char::code_point() const
{
    if (is_ascii_utf8_byte(xs[0]))
        return xs[0];
    if (!is_utf8_continuation_byte(xs[1]))
        return {};
    if (is_leading_byte_of_two_byte_utf8_seq(xs[0]))
        return (char32_t(xs[0] & 0x1f) << 6) + char32_t(xs[1] & 0x3f);
    if (!is_utf8_continuation_byte(xs[2]))
        return {};
    if (is_leading_byte_of_three_byte_utf8_seq(xs[0]))
        return (char32_t(xs[0] & 0x0f) << 12) + (char32_t(xs[1] & 0x3f) << 6) +
               char32_t(xs[1] & 0x3f);
    if (!is_utf8_continuation_byte(xs[3]))
        return {};
    if (is_leading_byte_of_four_byte_utf8_seq(xs[0]))
        return (char32_t(xs[0] & 0x07) << 18) + (char32_t(xs[1] & 0x3f) << 12) +
               (char32_t(xs[2] & 0x3f) << 6) + char32_t(xs[3] & 0x3f);
    return {};
}

}  // namespace forrest
