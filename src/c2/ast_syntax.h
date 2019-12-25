#pragma once

#include "util/utf.h"

namespace forrest {

const char OPEN_FNAPP_CHAR = '(';
const char CLOSE_FNAPP_CHAR = ')';
const char OPEN_LIST_CHAR = '[';
const char CLOSE_LIST_CHAR = ']';
const char STRING_QUOTE_CHAR = '"';
const char STRING_ESCAPE_CHAR = '\\';
const char COMMENT_CHAR = ';';
const string LAMBDA_ABSTRACTION_KEYWORD = "fn";
const string IGNORED_PARAMETER = "_";

// Must correspond to BUILTIN_NAMES in ast_syntax.cpp.
namespace ast {
enum class Builtin
{
    DATA,
    DEF,
    END_MARKER
};
}
bool is_symbol_char(Utf8Char x);
maybe<ast::Builtin> maybe_builtin_from_cstring(const char* s);
const char* to_cstring(ast::Builtin x);
bool is_variable_name(const string& s);
bool is_parameter_name(const string& s);
}  // namespace forrest
