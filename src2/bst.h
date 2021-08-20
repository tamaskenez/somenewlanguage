#pragma once
#include "common.h"
#include "type.h"

namespace snl {
struct TextAst;

struct Bst
{
    TypeStore ts;
    Bst();
};

optional<Bst> MakeBstFromTextAst(const TextAst& textAst);

}  // namespace snl
