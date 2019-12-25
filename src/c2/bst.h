#pragma once

#include <typeindex>

#include "ast.h"
#include "ast_syntax.h"
#include "ul/usual.h"

namespace forrest {

using namespace ul;

namespace bst {

struct Fn;

enum Type
{
    tString,
    tNumber,
    tFn,
    tFnapp,
    tTuple,
    tInstr,
    tVarname,
    tBuiltin
};

struct Expr
{
    Type type;
    const Fn* enclosing_fn;
    explicit Expr(Type type) : type(type) {}
    virtual ~Expr() {}
};

struct String : Expr
{
    string x;
    explicit String(string x) : Expr(tString), x(move(x)) {}
};

struct Number : Expr
{
    string x;
    explicit Number(string x) : Expr(tNumber), x(move(x)) {}
};

struct Varname : Expr
{
    string x;
    explicit Varname(string x) : Expr(tVarname), x(move(x)) {}
};

struct Builtin : Expr
{
    const ast::Builtin x;
    explicit Builtin(ast::Builtin x) : Expr(tBuiltin), x(x) {}
};

struct Instr : Expr
{
    enum Opcode
    {
        OP_READ_VAR,
        OP_CALL_FUNCTION
    };

    const Opcode opcode;
    const Expr* arg0 = nullptr;

    Instr(Opcode opcode, const Expr* arg0) : Expr(tInstr), opcode(opcode), arg0(arg0)
    {
        CHECK(opcode == OP_READ_VAR || opcode == OP_CALL_FUNCTION);
    }
};

string to_string(const Instr& x);

struct FnArg
{
    const Expr* value;
    explicit FnArg(const Expr* value) : value(value) {}
};

struct Fnapp : Expr
{
    using Args = vector<FnArg>;
    const Expr* fn_to_apply;
    Args args;
    Fnapp(const Expr* fn_to_apply, Args args)
        : Expr(tFnapp), fn_to_apply(fn_to_apply), args(move(args))
    {
        CHECK(!this->args.empty());
    }
};

struct FnPar
{
    string name;  // Empty means ignored parameter.
};

struct Fn : Expr
{
    using Pars = vector<FnPar>;
    Pars pars;
    const Expr* body;
    Fn(Pars pars, const Expr* body) : Expr(tFn), pars(move(pars)), body(body) {}
};

struct NamedExpr
{
    const string n;  // Name can be empty string, which means a positional-only expr.
    const Expr* x;
    explicit NamedExpr(const Expr* x) : x(x) {}
    NamedExpr(string n, const Expr* x) : n(move(n)), x(x) {}
};

vector<NamedExpr> vector_expr_to_vector_namedexpr(const vector<const Expr*>& xs);

struct Tuple : Expr
{
    const vector<NamedExpr> xs;
    const bool has_names;

    Tuple() : Expr(tTuple), has_names(false) {}
    explicit Tuple(vector<const Expr*> xs)
        : Expr(tTuple), xs(vector_expr_to_vector_namedexpr(xs)), has_names(false)
    {
    }
    explicit Tuple(vector<NamedExpr> xs)
        : Expr(tTuple),
          xs(move(xs)),
          has_names(std::any_of(BE(xs), [](auto& ne) { return !ne.n.empty(); }))
    {
    }
};

// TODO make sure these are used.
const auto EXPR_BUILTIN_DATA = Builtin{ast::Builtin::DATA};
const auto EXPR_BUILTIN_DEF = Builtin{ast::Builtin::DEF};
const auto EXPR_EMPTY_TUPLE = Tuple{};

struct TupleChain
{
    const Tuple* x = &EXPR_EMPTY_TUPLE;
    const TupleChain* parent = nullptr;
};

struct Env
{
    const Tuple* top_level_lexical;              // Defined in Bst;
    const vector<const Tuple*> opened_lexicals;  // These are defined elsewhere.
    const TupleChain
        local_chain;  // Subexpressions, local nonescaping lambdas inherit parent local scope chain.
    const TupleChain dynamic_chain;  // Callees inherit callers' dynamic scope chain.
    Env(const Tuple* top_level_lexical,
        const vector<const Tuple*> opened_lexicals,
        TupleChain local_chain,
        TupleChain dynamic_chain)
        : top_level_lexical(top_level_lexical),
          opened_lexicals(opened_lexicals),
          local_chain(local_chain),
          dynamic_chain(dynamic_chain)
    {
    }
};

template <class T>
const T* cast(const Expr* e)
{
    const T* t = dynamic_cast<const T*>(e);
    assert(t);
    return t;
}

template <class T>
T* cast(Expr* e)
{
    T* t = dynamic_cast<T*>(e);
    assert(t);
    return t;
}

}  // namespace bst

struct Bst
{
    const bst::Tuple* top_level_lexical_scope;
    const bst::Tuple* top_level_dynamic_scope;
    Bst(const bst::Tuple* top_level_lexical_scope, const bst::Tuple* top_level_dynamic_scope)
        : top_level_lexical_scope(top_level_lexical_scope),
          top_level_dynamic_scope(top_level_dynamic_scope)
    {
    }
};

const bst::Expr* process_ast(ast::Expr* e);
void dump(const bst::Expr* expr);
void dump_dfs(const bst::Expr* expr);
maybe<const bst::Expr*> lookup_by_name(const bst::Tuple* t, const string& n);

}  // namespace forrest
