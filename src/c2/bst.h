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
    static maybe<Builtin> maybe_from_string(const string& s);

    // Must correspond to BUILTIN_NAMES in bst.cpp.
    enum Name
    {
        TUPLE,
        VECTOR,
        FN,
        DATA,
        DEF,
        END_OF_NAMES_MARKER
    };
    const Name x;
    explicit Builtin(Name x) : x(x) {}
};

const char* to_cstring(Builtin::Name x);

struct Fnapp;
struct Instr;

using Expr = variant<String, Number, Fnapp, Varname, Builtin, Instr>;

struct Instr
{
    enum Opcode
    {
        OP_READ_VAR
    };

    const Opcode opcode;
    const Expr* arg0 = nullptr;

    Instr(Opcode opcode, const Expr* arg0) : opcode(opcode), arg0(arg0)
    {
        CHECK(opcode == OP_READ_VAR);
    }
};

string to_string(const Instr& x);

struct Fnapp
{
    const Expr* head;
    vector<const Expr*> args;
    vector<const Expr*> envargs;
    inline Fnapp(const Expr* head, vector<const Expr*> args);
    inline Fnapp(const Expr* head, vector<const Expr*> args, vector<const Expr*> envargs);
};

Fnapp::Fnapp(const Expr* head, vector<const Expr*> args, vector<const Expr*> envargs)
    : head(head), args(move(args)), envargs(move(envargs))
{
    CHECK(!this->args.empty());
}

Fnapp::Fnapp(const Expr* head, vector<const Expr*> args) : head(head), args(move(args))
{
    CHECK(!this->args.empty());
}

const auto BUILTIN_TUPLE = Expr{in_place_type<Builtin>, Builtin::TUPLE};
const auto BUILTIN_VECTOR = Expr{in_place_type<Builtin>, Builtin::VECTOR};
const auto BUILTIN_FN = Expr{in_place_type<Builtin>, Builtin::FN};
const auto BUILTIN_DATA = Expr{in_place_type<Builtin>, Builtin::DATA};
const auto BUILTIN_DEF = Expr{in_place_type<Builtin>, Builtin::DEF};
const auto NUMBER_0 = Expr{in_place_type<Number>, "0"};
const auto FNAPP_TUPLE_0 =
    Expr{in_place_type<Fnapp>, &BUILTIN_TUPLE, vector<const Expr*>{&NUMBER_0}};

enum Mutability
{
    IMMUTABLE,
    MUTABLE
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
    const bool removed;
    ImplicitVar(const Expr* x, Mutability m, bool removed) : x(x), m(m), removed(removed) {}
};

struct Env
{
    unordered_map<string, LocalVar> locals;
    unordered_map<string, ImplicitVar> implicits;
    const Env* parent_local_env = nullptr;
    const Env* parent_implicit_env = nullptr;

    void add_local(string name, LocalVar var)
    {
        auto itb = locals.insert(make_pair(move(name), move(var)));
        CHECK(name != ENV_NAME && itb.second, "Variable already exists.");
    }
    void add_implicit(string name, ImplicitVar var)
    {
        auto m = lookup_implicit(name);
        CHECK(!m, "Implicit variable already exists.");
        implicits.insert(make_pair(move(name), move(var)));
    }

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
    maybe<Var> lookup_implicit(const string& key) const
    {
        auto it = implicits.find(key);
        if (it != implicits.end()) {
            auto v = &(it->second);
            if (v->removed) {
                return {};
            }
            return Var{v->x, v->m};
        }
        if (parent_implicit_env) {
            return parent_local_env->lookup_local(key);
        }
        return {};
    }
};
const Env EMPTY_ENV;

}  // namespace bst

struct Bst
{
    deque<bst::Expr> exprs;
    deque<bst::Env> envs;
};

bst::Expr* process_ast(ast::Expr* e, Bst& bst);
void dump(bst::Expr* expr);
void dump_dfs(bst::Expr* expr);

// Return if exact value known at compile time.
inline bool statically_known(const bst::Expr* x)
{
    // TODO
    return true;
}

}  // namespace forrest
