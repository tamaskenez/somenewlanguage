#include "shell.h"

#include <span>
#include <variant>

#include "builtinnames.h"
#include "ul/usual.h"
#include "util/arena.h"

namespace forrest {

using std::get_if;

struct Lambda
{
    vector<const char*> pars;
    ExprPtr body;
};

using std::array;
using std::unique_ptr;

maybe<Lambda> try_get_fn_expr(ExprPtr e)
{
    auto p_apply_node = get_if<ApplyNode*>(&e);
    if (!p_apply_node)
        return {};
    auto apply_node = *p_apply_node;
    auto p_symleaf = get_if<SymLeaf*>(&(apply_node)->lambda);
    if (!p_symleaf)
        return {};
    auto symleaf = *p_symleaf;
    if (symleaf != BuiltinNames::g->id_to_symleaf(BuiltinNames::FN)) {
        return {};
    }
    auto& xs = apply_node->args->xs;
    if (xs.size() != 2)
        return {};
    auto p_args = get_if<TupleNode*>(&xs[0]);
    if (!p_args)
        return {};
    auto& args_xs = (*p_args)->xs;
    // args should be list of symbols
    vector<const char*> lambda_args;
    lambda_args.reserve(args_xs.size());
    for (auto& arg : args_xs) {
        auto p_arg = get_if<SymLeaf*>(&arg);
        if (!p_arg)
            return {};
        lambda_args.emplace_back((*p_arg)->name.c_str());
    }
    return Lambda{move(lambda_args), xs[1]};
};

maybe<ExprPtr> Shell::resolveSymbol(const string& name)
{
    auto mid = BuiltinNames::g->string_to_id(name);
    if (mid) {
        return BuiltinNames::g->id_to_symleaf(*mid);
    }
    auto it = symbols.find(name);
    if (it == symbols.end())
        return {};
    return it->second;
}

Shell::EvalResult Shell::eval(ExprPtr expr)
{
    Arena tmp_storage;
    return eval(expr, tmp_storage);
}

Shell::EvalResult beta_reduction(const Lambda& lambda, const vector<ExprPtr> evald_args)
{
    // todo
}

Shell::EvalResult Shell::eval_fn(const vector<ExprPtr>& evald_args)
{
    // @1 must be list of symbols
    // @2 must be body
}

Shell::EvalResult Shell::eval_def(const vector<ExprPtr>& evald_args)
{
    // @1 must be unset symbol
    // @2 must be value to set
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
                auto er = visit(*this, x);
                if (is_left(er))
                    return left(er);
                evald_xs.emplace_back(right(er));
            }
            // Here we have an evaluated tuple
            return storage.new_<TupleNode>(BE(evald_xs));
        }
        EvalResult operator()(StrNode* p) { return p; }
        EvalResult operator()(SymLeaf* p)
        {
            auto me = shell.resolveSymbol(p->name);
            if (me) {
                return *me;
            }
            return EvalError{string("Unknown symbol: ") + p->name};
        }
        EvalResult operator()(NumLeaf* p) { return p; }
        EvalResult operator()(CharLeaf* p) { return p; }
        EvalResult operator()(ApplyNode* p)
        {
            vector<ExprPtr> evald_args;
            auto args_xs = p->args->xs;
            evald_args.reserve(args_xs.size());
            for (auto e : p->args->xs) {
                auto er = visit(*this, e);
                if (is_left(er)) {
                    return er;
                }
                evald_args.emplace_back(right(er));
            }

            auto er_lambda = visit(*this, p->lambda);
            if (is_left(er_lambda))
                return er_lambda;
            // Check if builtin.
            auto evald_lambda = right(er_lambda);
            auto p_symleaf = get_if<SymLeaf*>(&evald_lambda);
            if (p_symleaf) {
                auto sl = *p_symleaf;
                if (sl == BuiltinNames::g->id_to_symleaf(BuiltinNames::FN)) {
                    return shell.eval_fn(evald_args);
                } else if (sl == BuiltinNames::g->id_to_symleaf(BuiltinNames::DEF)) {
                    return shell.eval_def(evald_args);
                }
            }
            auto m_lambda = try_get_fn_expr(evald_lambda);
            if (!m_lambda) {
                return EvalError{"First element is not a lambda."};
            };

            // Bind evald_args to lambda.pars in lambda.body.
            // The can be less args then pars, then it's currying.
            return beta_reduction(*m_lambda, evald_args);
        }
        EvalResult operator()(QuoteNode* p) { return p->expr; }
    };
    return visit(Visitor{*this, storage}, expr);
}

}  // namespace forrest