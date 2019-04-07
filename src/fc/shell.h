#pragma once

#include "util/arena.h"
#include "util/either.h"

#include "ast.h"

namespace forrest {

struct Shell
{
    struct EvalError {string msg;};
    using EvalResult = either<EvalError, ExprPtr>;

    EvalResult eval(ExprPtr expr);
    EvalResult eval(ExprPtr expr, Arena& storage);
};

}  // namespace forrest