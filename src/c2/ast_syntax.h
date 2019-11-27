#pragma once

#include "util/utf.h"

namespace forrest {

const char OPEN_LIST_CHAR = '(';
const char CLOSE_LIST_CHAR = ')';
const char STRING_QUOTE_CHAR = '"';
const char STRING_ESCAPE_CHAR = '\\';
const char COMMENT_CHAR = ';';

bool is_symbol_char(Utf8Char x);

}  // namespace forrest
