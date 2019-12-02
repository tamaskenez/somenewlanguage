#pragma once

#include "bst.h"

namespace forrest {

struct Cst
{};
const bst::Expr* compile(const bst::Fnapp* e, Bst& bst, const bst::Env* env);

}  // namespace forrest
