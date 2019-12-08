#pragma once

#include "ast.h"
#include "ast_syntax.h"
#include "ul/usual.h"

namespace forrest {

using namespace ul;

namespace bst {

struct String
{
    string x;
    explicit String(string x) : x(move(x)) {}
};

struct Number
{
    string x;
    explicit Number(string x) : x(move(x)) {}
};

struct Varname
{
    string x;
    explicit Varname(string x) : x(move(x)) {}
};

struct Builtin
{
    const forrest::Builtin x;
    explicit Builtin(forrest::Builtin x) : x(x) {}
};

struct Fnapp;
struct Instr;
struct List;
struct Fn;

using Expr = variant<Builtin, Number, String, Varname, List, Fn, Fnapp, Instr>;

struct Instr
{
    enum Opcode
    {
        OP_READ_VAR,
        OP_CALL_FUNCTION
    };

    const Opcode opcode;
    const Expr* arg0 = nullptr;

    Instr(Opcode opcode, const Expr* arg0) : opcode(opcode), arg0(arg0)
    {
        CHECK(opcode == OP_READ_VAR || opcode == OP_CALL_FUNCTION);
    }
};

string to_string(const Instr& x);

struct FnArg
{
    string name;  // Empty if positional.
    const Expr* value;

    FnArg(string name, const Expr* value) : name(move(name)), value(value) {}
    bool positional() const { return name.empty(); }
};

struct Fnapp
{
    using Args = vector<FnArg>;
    const Expr* fn_to_apply;
    Args args, envargs;
    inline Fnapp(const Expr* fn_to_apply, Args args);
    inline Fnapp(const Expr* fn_to_apply, Args args, Args envargs);
};

struct FnPar
{
    string name;  // Empty means ignored parameter.
};

struct Fn
{
    using Pars = vector<FnPar>;
    Pars pars, envpars;
    const Expr* body;
    Fn(Pars pars, Pars envpars, const Expr* body)
        : pars(move(pars)), envpars(move(envpars)), body(body)
    {
    }
};

struct List
{
    vector<const Expr*> xs;

    List() = default;
    inline explicit List(vector<const Expr*> xs);
};

Fnapp::Fnapp(const Expr* fn_to_apply, Args args, Args envargs)
    : fn_to_apply(fn_to_apply), args(move(args)), envargs(move(envargs))
{
    CHECK(!this->args.empty());
}

Fnapp::Fnapp(const Expr* fn_to_apply, Args args) : fn_to_apply(fn_to_apply), args(move(args))
{
    CHECK(!this->args.empty());
}

List::List(vector<const Expr*> xs) : xs(move(xs)) {}

// TODO make sure these are used.
const auto EXPR_BUILTIN_DATA = Expr{in_place_type<Builtin>, forrest::Builtin::DATA};
const auto EXPR_BUILTIN_DEF = Expr{in_place_type<Builtin>, forrest::Builtin::DEF};
const auto EXPR_EMPTY_LIST = Expr{in_place_type<List>};

enum Mutability
{
    IMMUTABLE,
    MUTABLE
};

enum ImplicitAccess
{
    ACCESS_THROUGH_ENV,
    ACCESS_AS_LOCAL,
    WITHDRAW_ACCESS
};

struct Var
{
    const Expr* x;
    const Mutability m;
    Var(const Expr* x, Mutability m) : x(x), m(m) {}
};

struct LocalVar
{
    const Expr* x;
    const Mutability m;
    LocalVar(const Expr* x, Mutability m) : x(x), m(m) {}
};

struct ImplicitVar
{
    const Expr* x;
    const Mutability m;
    const ImplicitAccess a;
    ImplicitVar(const Expr* x, Mutability m, ImplicitAccess a) : x(x), m(m), a(a) {}
};

struct Env
{
    static Env create_for_fnapp(const Env* caller_env) { return Env(nullptr, caller_env); }

    const Env* parent_local_env = nullptr;
    const Env* parent_implicit_env = nullptr;
    unordered_map<string, LocalVar> locals;
    unordered_map<string, ImplicitVar> implicits;

    Env() = default;
    Env(const Env* parent_local_env, const Env* parent_implicit_env)
        : parent_local_env(parent_local_env), parent_implicit_env(parent_implicit_env)
    {
    }

    void add_local(string name, LocalVar var)
    {
        auto itb = locals.insert(make_pair(move(name), move(var)));
        CHECK(itb.second, "Variable already exists.");
    }
    void add_implicit(string name, ImplicitVar var)
    {
        auto m = lookup_implicit(name, true);
        CHECK(!m, "Implicit variable already exists.");
        implicits.insert(make_pair(move(name), move(var)));
    }

    // Look up local vars and implicit vars with ACCESS_AS_LOCAL
    maybe<Var> lookup_as_local(const string& key) const
    {
        auto m = lookup_local(key);
        if (m) {
            return m;
        }
        return lookup_implicit(key, false);
    }
    maybe<Var> lookup_through_env(const string& key) const { return lookup_implicit(key, true); }
    maybe<Var> lookup_local(const string& key) const
    {
        auto it = locals.find(key);
        if (it != locals.end()) {
            auto v = &(it->second);
            return Var{v->x, v->m};
        }
        if (parent_local_env) {
            return parent_local_env->lookup_local(key);
        }
        return {};
    }
    maybe<Var> lookup_implicit(const string& key, bool access_through_env) const
    {
        auto it = implicits.find(key);
        if (it != implicits.end()) {
            auto v = &(it->second);
            if (v->a == WITHDRAW_ACCESS) {
                return {};
            }
            if (access_through_env || v->a == ACCESS_AS_LOCAL) {
                return Var{v->x, v->m};
            }
            return {};
        }
        if (parent_implicit_env) {
            return parent_local_env->lookup_implicit(key, access_through_env);
        }
        return {};
    }
};
const Env EMPTY_ENV;

}  // namespace bst

struct Bst
{
    deque<bst::Expr> exprs;
};

const bst::Expr* process_ast(ast::Expr* e, Bst& bst);
void dump(const bst::Expr* expr);
void dump_dfs(const bst::Expr* expr);

// Return if exact value known at compile time.
inline bool statically_known(const bst::Expr* x)
{
    // TODO
    return true;
}

}  // namespace forrest
