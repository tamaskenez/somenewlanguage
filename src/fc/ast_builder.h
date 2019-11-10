#pragma once

#include <memory>

#include "util/maybe.h"

#include "ast.h"

namespace forrest {

class FileReader;

using std::unique_ptr;
using std::vector;

class Arena;

namespace AstBuilder {
maybe<vector<Node*>> parse_filereader_into_ast(FileReader& fr, Arena& storage);
}

}  // namespace forrest
