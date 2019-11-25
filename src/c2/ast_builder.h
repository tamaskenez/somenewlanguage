#pragma once

#include <memory>

#include "ul/maybe.h"

#include "ast.h"

namespace forrest {

class FileReader;

using std::unique_ptr;
using std::vector;

using ul::maybe;

class Arena;

namespace AstBuilder {
maybe<vector<Node*>> parse_filereader_into_ast(FileReader& fr, Arena& storage);
}

}  // namespace forrest
