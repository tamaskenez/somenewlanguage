#pragma once

#include <memory>

#include "ul/either.h"

#include "ast.h"

namespace forrest {

class FileReader;

using std::unique_ptr;
using std::vector;

using ul::either;

class Arena;

namespace AstBuilder {
// Return top-level expressions.
either<string, vector<ast::Expr*>> parse_filereader_into_ast(FileReader& fr, Ast& ast);
}  // namespace AstBuilder

}  // namespace forrest
