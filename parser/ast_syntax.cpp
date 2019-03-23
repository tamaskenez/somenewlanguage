#include "ast_syntax.h"

namespace forrest {

static const char SPECIAL_CHARS[] = {OPEN_VEC_CHAR,
                                     CLOSE_VEC_CHAR,
                                     STRING_QUOTE_CHAR,
                                     STRING_ESCAPE_CHAR,
                                     OPEN_AVEC_CHAR,
                                     CLOSE_AVEC_CHAR,
                                     0};

bool is_symbol_char(Utf8Char x)
{
    auto p = SPECIAL_CHARS;
    do {
        if (*p == x.xs[0])
            return false;
    } while (*(p++) != 0);
    return !iscntrl(x.xs[0]) && !isspace(x.xs[0]);
}

}  // namespace forrest
