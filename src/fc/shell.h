#pragma once

#include <unordered_map>

#include <util/maybe.h>
#include "util/arena.h"
#include "util/either.h"

#include "ast.h"

namespace forrest {

using std::unordered_map;

struct Shell
{
    struct EvalError
    {
        string msg;
    };
    using EvalResult = either<EvalError, ExprPtr>;

    maybe<ExprPtr> resolveSymbol(const string& name);
    EvalResult eval(ExprPtr expr);
    EvalResult eval(ExprPtr expr, Arena& storage);

private:
    unordered_map<string, ExprPtr> symbols;

    EvalResult eval_fn(const vector<ExprPtr>& evald_args);
    EvalResult eval_def(const vector<ExprPtr>& evald_args);
};

}  // namespace forrest