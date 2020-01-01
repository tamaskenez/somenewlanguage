#include "cst.h"

namespace forrest {

const cst::Expr* compile_function(const bst::Expr* e, vector<const bst::Expr*> args)
{
    CHECK(!args.empty());
    // e must be fn... or can it be anything callable? Shouldn't we have transformed it to fn
    // before?
    return nullptr;
}

#if 0
const bst::Expr* compile(const bst::Expr* e, Bst& bst, const bst::Env* env)
{
    using namespace bst;
    switch (e->type) {
        case tString:
        case tNumber:
            return e;
        case tVarname:
            /*
            auto m_vc = env->lookup_as_local(vn->x);
                CHECK(m_vc, "Unknown variable: `%s`", CSTR vn->x);
            auto vc = *m_vc;
            if (statically_known(vc.x)) {
                return vc.x;
            } else {
                return &bst.exprs.emplace_back(in_place_type<bst::Instr>, bst::Instr::OP_READ_VAR,
            e);
            }
             */
            UL_UNREACHABLE;
        case tFnapp:
            return compile_fnapp(cast<Fnapp>(e), bst, env);
        case tTuple: {
            auto t = cast<Tuple>(e);
            if (t->xs.empty()) {
                return e;
            }
            UL_UNREACHABLE;
        }
        case tFn:
        case tInstr:
        case tBuiltin:
            UL_UNREACHABLE;
    }
}

const bst::Fn* transform_to_fn(const bst::Expr* e)
{
    using namespace bst;
    switch (e->type) {
        case tBuiltin: {
            auto bi = cast<Builtin>(e);
            switch (bi->head) {
                case ast::Builtin::FN: {
                    auto t = bi->xs;
                    auto xs = t->xs;
                    CHECK(~xs == 2);
                    auto pars = cast<Tuple>(xs[0].x);
                    vector<FnPar> fnpars;
                    fnpars.reserve(~pars->xs);
                    for (auto p : pars->xs) {
                        fnpars.emplace_back(FnPar{cast<String>(p.x)->x});
                    }
                    return new Fn(fnpars, xs[1].x);
                }
                default:
                    UL_UNREACHABLE;
            }
        }
        default:
            UL_UNREACHABLE;
    }
    /*
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
     */
    UL_UNREACHABLE;
    return nullptr;
}

void call_make_cenv(const string& s)
{
    int a = 3;
    UL_UNREACHABLE;
}

// Assume the application is invoked at run time. Generate the instructions to evaluate which are
// going to perform the evaluation.
const bst::Expr* compile_fnapp(const bst::Fnapp* e, Bst& bst, const bst::Env* env)
{
    auto fn = transform_to_fn(e->fn_to_apply);
    CHECK(!e->args.empty());
    // TODO unused args without side effects must only be syntax/type-checked and ignored.
    auto arg0 = compile(e->args[0].value, bst, env);
    if (fn->pars.empty()) {
        // It means it expects a unit.
        using namespace bst;
        auto p = cast<Tuple>(arg0);
        CHECK(p->xs.empty());

        // top level lexical is kept
        // opened lexicals inherited from enclosing function (or none if toplevel)
        // local chain is inherited from enclosing function (or none if toplevel)
        // dynamic_chain is inherited from caller.
        // So for every fn node we need to remember the caller's local chain and opened lexicals

        return compile(fn->body, bst, env);
    } else {
        UL_UNREACHABLE;
    }
#if 0
    if (fn->pars.empty()) {
    } else {  // ... else fn has actual parameters
        if (auto instr = get_if<bst::Instr>(fn->body)) {
            CHECK(instr->opcode == bst::Instr::OP_CALL_FUNCTION);
            CHECK(~fn->pars == 1);
            auto& s = get<bst::String>(*arg0);
            call_make_cenv(s.x);
            UL_UNREACHABLE;
        }
        UL_UNREACHABLE;
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
#endif
    UL_UNREACHABLE;
    return nullptr;
}
#endif
}  // namespace forrest
