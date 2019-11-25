#pragma once

#include <unordered_map>

#include "ul/maybe.h"
#include "ul/either.h"

#include "util/arena.h"

#include "ast.h"

namespace forrest {

using std::unordered_map;
using ul::maybe;
using ul::either;

struct Shell
{
    struct EvalError
    {
        string msg;
    };
    using EvalResult = either<EvalError, Node*>;

    maybe<Node*> resolveSymbol(const string& name);
    EvalResult eval(Node* expr);
    EvalResult eval(Node* expr, Arena& storage);

    unordered_map<string, Node*> symbols;

private:
    EvalResult eval_fn(const vector<Node*>& evald_args);
    EvalResult eval_def(const vector<Node*>& evald_args);
};

}  // namespace forrest
