#include "cst.h"

namespace forrest {

const bst::Expr* compile(const bst::Expr* e, Bst& bst, const bst::Env* env)
{
    if (auto fn = get_if<bst::Fn>(e)) {
        UL_UNREACHABLE;
    } else if (auto fnapp = get_if<bst::Fnapp>(e)) {
        return compile(fnapp, bst, env);
    } else if (auto n = get_if<bst::Number>(e)) {
        return e;
    } else if (auto b = get_if<bst::Builtin>(e)) {
        return e;
    } else if (auto vn = get_if<bst::Varname>(e)) {
        auto m_vc = env->lookup_as_local(vn->x);
        CHECK(m_vc, "Unknown variable: `%s`", CSTR vn->x);
        auto vc = *m_vc;
        if (statically_known(vc.x)) {
            return vc.x;
        } else {
            return &bst.exprs.emplace_back(in_place_type<bst::Instr>, bst::Instr::OP_READ_VAR, e);
        }
    } else if (auto s = get_if<bst::String>(e)) {
        return e;
    } else if (auto l = get_if<bst::List>(e)) {
        if (l->xs.empty()) {
            return e;
        } else {
            UL_UNREACHABLE;
        }
    } else {
        // TODO
        UL_UNREACHABLE;
    }
}

const bst::Expr* transform_to_fn(const bst::Expr* e, Bst& bst, const bst::Env* env)
{
    if (holds_alternative<bst::Fn>(*e)) {
        return e;
    } else if (auto vn = get_if<bst::Varname>(e)) {
        // TODO put this into env
        if (vn->x == "make-cenv") {
            vector<const bst::Expr*> xs{
                &bst.exprs.emplace_back(in_place_type<bst::String>, "filename")};
            auto list_of_args = &bst.exprs.emplace_back(in_place_type<bst::List>, xs);
            auto body = &bst.exprs.emplace_back(in_place_type<bst::Instr>,
                                                bst::Instr::OP_CALL_FUNCTION, list_of_args);
            return &bst.exprs.emplace_back(in_place_type<bst::Fn>,
                                           vector<bst::FnPar>{bst::FnPar{"filename"}},
                                           vector<bst::FnPar>{}, body);
        }
        auto m_vc = env->lookup_local(vn->x);
        CHECK(m_vc);
        auto& vc = *m_vc;
        auto ve = vc.x;
        return transform_to_fn(vc.x, bst, env);
    }
    UL_UNREACHABLE;
    return nullptr;
}

// Assume the application is invoked at run time. Generate the instructions to evaluate which are
// going to perform the evaluation.
const bst::Expr* compile(const bst::Fnapp* e, Bst& bst, const bst::Env* env)
{
    CHECK(!e->args.empty());
    CHECK(e->envargs.empty());  // TODO: For now let's forget about envargs.
    auto fn = get_if<bst::Fn>(transform_to_fn(e->fn_to_apply, bst, env));
    CHECK(e->args[0].name.empty());  // TODO implement named parameters.
    auto arg0 = compile(e->args[0].value, bst, env);
    CHECK(fn->envpars.empty());
    if (fn->pars.empty()) {
        // It means it expects a unit.
        auto p = get_if<bst::List>(arg0);
        CHECK(p && p->xs.empty());
        return compile(fn->body, bst, env);
    } else {  // ... else fn has actual parameters
        int a = 3;
        /*
        bst::Env new_env =
            open_new_local_scope ? env->new_local_scope() : env->new_scope_keep_locals();
        new_env.add_local(dereference_if_variable(fn->pars[0].name, bst),
                          bst::LocalVar{arg0, bst::IMMUTABLE});
        if (~e->args > 1) {
            CHECK(fn && !fn->pars.empty());
            auto fnred = &bst.exprs.emplace_back(
                in_place_type<bst::Fn>,
                vector<bst::FnPar>{fn->pars.begin() + 1, fn->pars.end()}, vector<bst::FnPar>{},
                fn->body);
            auto q =
                bst::Fnapp{fnred, vector<bst::FnArg>(e->args.begin() + 1, e->args.end()), {}};
            auto fnredapp = &bst.exprs.emplace_back(
                in_place_type<bst::Fnapp>, fnred,
                vector<bst::FnArg>(e->args.begin() + 1, e->args.end()), {});
            return compile(fnredapp, bst, &new_env);
        } else {
            // All arguments done.
            return fnred;
        }
         */
    }
    UL_UNREACHABLE;
    return nullptr;
}

}  // namespace forrest
