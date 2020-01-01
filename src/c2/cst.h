#pragma once

#include "bst.h"

namespace forrest {

namespace cst {

enum Type
{
};

struct Expr
{
    Type type;
    explicit Expr(Type type) : type(type) {}
    virtual ~Expr() {}
};

}  // namespace cst

// 'e' must be Fn
const cst::Expr* compile_function(const bst::Expr* e, vector<const bst::Expr*> args);

/*
const bst::Expr* compile(const bst::Expr* e, Bst& bst, const bst::Env* env);
const bst::Expr* compile_fnapp(const bst::Fnapp* e, Bst& bst, const bst::Env* env);
*/

}  // namespace forrest
