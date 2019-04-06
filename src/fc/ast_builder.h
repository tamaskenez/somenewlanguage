#pragma once

#include <memory>

namespace forrest {

class FileReader;
struct Ast;

using std::unique_ptr;

namespace AstBuilder {
bool parse_filereader_into_ast(FileReader& fr, Ast& ast);
}

}  // namespace forrest
