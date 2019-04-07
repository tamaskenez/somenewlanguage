#include "shell.h"

#include <span>

#include "ul/usual.h"
#include "util/arena.h"


// {def `a `{fn (`x) `{print x}}}
// a -> {fn (x) `{print x}}}


namespace forrest {

    using std::span;

    struct FnExpr
    {
        span<ExprPtr> args;
        ExprPtr body;
    };

Shell::EvalResult Shell::eval(ExprPtr expr)
{
    Arena tmp_storage;
    return eval(expr, tmp_storage);
}

Shell::EvalResult Shell::eval(ExprPtr expr, Arena& storage)
{
    struct Visitor
    {
        Shell& shell;
        Arena& storage;
        explicit Visitor(Shell& shell, Arena& storage) : shell(shell), storage(storage) {}

        EvalResult operator()(TupleNode* p)
        {
            vector<ExprPtr> evald_xs;
            for (auto x : p->xs) {
                evald_xs.emplace_back(shell.eval(x));
            }
            // Here we have an evaluated tuple
            return storage.new_<TupleNode>(BE(evald_xs));
        }
        EvalResult operator()(StrNode* p) { return p; }
        EvalResult operator()(SymLeaf* p)
        {
            // look up symleaf
        }
        EvalResult operator()(NumLeaf* p) { return p; }
        EvalResult operator()(CharLeaf* p) { return p; }
        EvalResult operator()(ApplyNode* p)
        {
            auto xs = p->tuple->xs;
            if (xs.empty()) {
                return EvalError{"Can't eval empty tuple."};
            };
            auto function = xs.front();
            // Look up function, function must be a (fn args body) expression.
             auto fn_expr = try_to_fn_expr(xs.front());
            if (holds_alternative
        }
        EvalResult operator()(QuoteNode* p) { return p->expr; }
    };
    visit(Visitor{*this, storage}, expr);
}

}  // namespace forrest