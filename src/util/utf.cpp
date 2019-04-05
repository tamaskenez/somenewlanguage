#include "util/utf.h"

#include "absl/strings/str_format.h"
#include "ul/usual.h"

namespace forrest {

using absl::StrFormat;
using namespace ul;

string utf32_to_descriptive_string(char32_t cp)
{
    if (cp <= 0x7f) {
        return isgraph(cp) ? StrFormat("'%c", cp) : StrFormat("U+%02X", cp);
    } else {
        return StrFormat("U+%04X", cp);
    }
}

string to_descriptive_string(Utf8Char c)
{
    auto cp = c.code_point();
    if (cp)
        return utf32_to_descriptive_string(*cp);
    else
        return string("<invalid UTF-8>");
}

int Utf8Char::size() const
{
    if (is_ascii_utf8_byte(xs[0]))
        return 1;
    do {
        if (!is_utf8_continuation_byte(xs[1]))
            break;
        if (is_leading_byte_of_two_byte_utf8_seq(xs[0]))
            return 2;
        if (!is_utf8_continuation_byte(xs[2]))
            break;
        if (is_leading_byte_of_three_byte_utf8_seq(xs[0]))
            return 3;
        if (is_utf8_continuation_byte(xs[3]) && is_leading_byte_of_four_byte_utf8_seq(xs[0]))
            return 4;
    } while (false);
    assert(false);
    return 0;
}

maybe<char32_t> Utf8Char::code_point() const
{
    if (is_ascii_utf8_byte(xs[0]))
        return xs[0];
    do {
        if (!is_utf8_continuation_byte(xs[1]))
            break;
        ;
        if (is_leading_byte_of_two_byte_utf8_seq(xs[0]))
            return (char32_t(xs[0] & 0x1f) << 6) + char32_t(xs[1] & 0x3f);
        if (!is_utf8_continuation_byte(xs[2]))
            break;
        if (is_leading_byte_of_three_byte_utf8_seq(xs[0]))
            return (char32_t(xs[0] & 0x0f) << 12) + (char32_t(xs[1] & 0x3f) << 6) +
                   char32_t(xs[1] & 0x3f);
        if (is_utf8_continuation_byte(xs[3]) && is_leading_byte_of_four_byte_utf8_seq(xs[0]))
            return (char32_t(xs[0] & 0x07) << 18) + (char32_t(xs[1] & 0x3f) << 12) +
                   (char32_t(xs[2] & 0x3f) << 6) + char32_t(xs[3] & 0x3f);
    } while (false);
    assert(false);
    return {};
}

}  // namespace forrest
