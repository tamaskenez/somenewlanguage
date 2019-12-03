#include "ast_syntax.h"

#include "ul/check.h"

namespace forrest {

static const char SPECIAL_CHARS[] = {OPEN_LIST_CHAR,     CLOSE_LIST_CHAR, STRING_QUOTE_CHAR,
                                     STRING_ESCAPE_CHAR, COMMENT_CHAR,    0};

bool is_symbol_char(Utf8Char x)
{
    auto p = SPECIAL_CHARS;
    do {
        if (*p == x.front())
            return false;
    } while (*(p++) != 0);
    return !iscntrl(x.front()) && !isspace(x.front());
}

// Must correspond to enum values, terminate with zero.
const char* BUILTIN_NAMES[] = {"tuple", "vector", "fn", "data", "def", "env", 0};

const char* to_cstring(Builtin x)
{
    return BUILTIN_NAMES[static_cast<int>(x)];
}

maybe<Builtin> maybe_builtin_from_cstring(const char* s)
{
    for (auto p = BUILTIN_NAMES; *p; ++p) {
        if (strcmp(s, *p) == 0) {
            auto i = p - BUILTIN_NAMES;
            CHECK(0 <= i && i < static_cast<int>(Builtin::END_MARKER));
            return Builtin{static_cast<Builtin>(i)};
        }
    }
    return {};
}

}  // namespace forrest
