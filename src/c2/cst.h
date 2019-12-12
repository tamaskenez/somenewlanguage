#pragma once

#include "bst.h"

namespace forrest {

const bst::Expr* compile(const bst::Expr* e, Bst& bst, const bst::Env* env);
const bst::Expr* compile_fnapp(const bst::Fnapp* e, Bst& bst, const bst::Env* env);

}  // namespace forrest
