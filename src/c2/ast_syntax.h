#pragma once

#include "util/utf.h"

namespace forrest {

const char OPEN_LIST_CHAR = '(';
const char CLOSE_LIST_CHAR = ')';
const char STRING_QUOTE_CHAR = '"';
const char STRING_ESCAPE_CHAR = '\\';
const char COMMENT_CHAR = ';';
const string ENV_ARGS_SEPARATOR = "%";

// Must correspond to BUILTIN_NAMES in ast_syntax.cpp.
enum class Builtin
{
    TUPLE,
    VECTOR,
    FN,
    DATA,
    DEF,
    ENV,
    END_MARKER
};

bool is_symbol_char(Utf8Char x);
maybe<Builtin> maybe_builtin_from_cstring(const char* s);
const char* to_cstring(Builtin x);

}  // namespace forrest
