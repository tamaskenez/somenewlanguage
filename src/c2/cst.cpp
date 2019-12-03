#include "cst.h"

namespace forrest {

const bst::Expr* compile(const bst::Expr* e, Bst& bst, const bst::Env* env)
{
    if (auto fnapp = get_if<bst::Fnapp>(e)) {
        if (auto bi = get_if<bst::Builtin>(fnapp->fn_to_apply)) {
            if (bi->x == Builtin::FN) {
                // Compile fn expressions at fn-application.
                return e;
            }
        }
        return compile(fnapp, bst, env);
    } else if (auto n = get_if<bst::Number>(e)) {
        return e;
    } else if (auto b = get_if<bst::Builtin>(e)) {
        return e;
    } else if (auto vn = get_if<bst::Varname>(e)) {
        auto m_vc = env->lookup_local(vn->x);
        CHECK(m_vc, "Unknown variable: `%s`", CSTR vn->x);
        auto vc = *m_vc;
        if (statically_known(vc.x)) {
            return vc.x;
        } else {
            return &bst.exprs.emplace_back(in_place_type<bst::Instr>, bst::Instr::OP_READ_VAR, e);
        }
    } else if (auto s = get_if<bst::String>(e)) {
        return e;
    } else {
        // TODO
        UL_UNREACHABLE;
    }
}

// Assume the application is invoked at run time. Generate the instructions to evaluate which are
// going to perform the evaluation.
const bst::Expr* compile(const bst::Fnapp* e, Bst& bst, const bst::Env* env)
{
    UL_UNREACHABLE;
#if 0
    // Compile args
    vector<const bst::Expr*> c_args, c_envargs;
    for (auto a : e->args) {
        c_args.push_back(compile(a, bst, env));
    }
    for (auto a : e->envargs) {
        c_envargs.push_back(compile(a, bst, env));
    }
    // Compile head
    auto c_head = compile(e->head, bst, env);

    // Compile application
    // We're applying e->args and e->env_args to e->head
    if (auto c_head_as_fnapp = get_if<bst::Fnapp>(c_head)) {
        if (auto bi = get_if<bst::Builtin>(c_head_as_fnapp->head)) {
            int a = 3;
            switch (bi->x) {
                case Builtin::FN: {
                    // Create new env:
                    // New locals: parameters replaced by arguments
                    // + env parameters replaced by env arguments
                    // New implicits: parent implicits.
                    auto new_env = bst::Env::create_for_fnapp(env);
                    auto min_envargs = std::min(~e->envargs, ~c_head_as_fnapp->envargs);
                    for (int i = 0; i < min_envargs; ++i) {
                        auto env_par = c_head_as_fnapp->envargs[i];
                        auto s = get_if<bst::String>(env_par);
                        CHECK(s);
                        auto env_arg = e->envargs[i];
                        new_env.add_local(s->x, bst::LocalVar{env_arg, bst::IMMUTABLE});
                    }
                    auto min_args = std::min(~e->args, ~c_head_as_fnapp->args);
                    for (int i = 0; i < min_args; ++i) {
                        auto par = c_head_as_fnapp->args[i];
                        auto s = get_if<bst::String>(par);
                        CHECK(s);
                        auto arg = e->args[i];
                        new_env.add_local(s->x, bst::LocalVar{arg, bst::IMMUTABLE});
                    }
                } break;
                case Builtin::TUPLE:
                case Builtin::VECTOR:
                case Builtin::DATA:
                case Builtin::DEF:
                case Builtin::ENV:
                default:
                    UL_UNREACHABLE;
            }
        }
    } else if (auto bi = get_if<bst::Builtin>(c_head)) {
        switch (bi->x) {
            case Builtin::TUPLE:
            case Builtin::VECTOR:
                return &bst.exprs.emplace_back(in_place_type<bst::Fnapp>, c_head, c_args,
                                               c_envargs);
            case Builtin::DATA:
            case Builtin::FN:
            case Builtin::DEF:
            case Builtin::ENV:
            default:
                UL_UNREACHABLE;
        }
        UL_UNREACHABLE;
    } else {
        // TODO
        UL_UNREACHABLE;
    }
    UL_UNREACHABLE;
#endif
}

}  // namespace forrest
