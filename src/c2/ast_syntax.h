#pragma once

#include "util/utf.h"

namespace forrest {

const char OPEN_TUPLE_CHAR = '(';
const char CLOSE_TUPLE_CHAR = ')';
const char STRING_QUOTE_CHAR = '"';
const char STRING_ESCAPE_CHAR = '\\';
const char OPEN_APPLY_CHAR = '{';
const char CLOSE_APPLY_CHAR = '}';
const char UTFCHAR_PREFIX = '\'';
const char QUOTE_CHAR = '`';

bool is_symbol_char(Utf8Char x);

}  // namespace forrest
