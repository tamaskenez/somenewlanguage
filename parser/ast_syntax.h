#pragma once

#include "util/utf8.h"

namespace forrest {

const char OPEN_VEC_CHAR = '(';
const char CLOSE_VEC_CHAR = ')';
const char STRING_QUOTE_CHAR = '"';
const char STRING_ESCAPE_CHAR = '\\';
const char OPEN_AVEC_CHAR = '{';
const char CLOSE_AVEC_CHAR = '}';
const char UTFCHAR_PREFIX = '\'';

bool is_symbol_char(Utf8Char x);

}  // namespace forrest
