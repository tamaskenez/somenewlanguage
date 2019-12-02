#include "cst.h"

#define CSTR TO_CSTR_CONVERTER{} %
struct TO_CSTR_CONVERTER
{};
const char* operator%(const TO_CSTR_CONVERTER& x, const std::string& y)
{
    return y.c_str();
}

namespace forrest {

const bst::Expr* compile(const bst::Expr* e, Bst& bst, const bst::Env* env)
{
    if (auto fnapp = get_if<bst::Fnapp>(e)) {
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
    // Compile args
    vector<const bst::Expr*> c_args, c_envargs;
    for (auto a : e->args) {
        c_args.push_back(compile(a, bst, env));
    }
    for (auto a : e->envargs) {
        c_envargs.push_back(compile(a, bst, env));
    }
    auto c_head = compile(e->head, bst, env);
    // Compile head
    // Compile application
}

}  // namespace forrest
