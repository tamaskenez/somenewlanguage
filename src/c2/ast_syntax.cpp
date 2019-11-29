#include "ast_syntax.h"

namespace forrest {

static const char SPECIAL_CHARS[] = {OPEN_LIST_CHAR,     CLOSE_LIST_CHAR, STRING_QUOTE_CHAR,
                                     STRING_ESCAPE_CHAR, COMMENT_CHAR,    0};

static const char* BUILTINS[] = {"fn", 0};

bool is_symbol_char(Utf8Char x)
{
    auto p = SPECIAL_CHARS;
    do {
        if (*p == x.front())
            return false;
    } while (*(p++) != 0);
    return !iscntrl(x.front()) && !isspace(x.front());
}

bool is_builtin(const char* s)
{
    for (auto p = BUILTINS; *p; ++p) {
        if (strcmp(*p, s) == 0) {
            return true;
        }
    }
    return false;
}

}  // namespace forrest
