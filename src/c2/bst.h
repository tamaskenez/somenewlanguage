#pragma once

#include "ast.h"
#include "ast_syntax.h"
#include "ul/usual.h"

namespace forrest {

using namespace ul;

namespace bst {

struct Expr
{
    // Values must correspond to the types in ExprTypes below.
    enum Type
    {
        STRING,
        NUMBER,
        VARNAME,
        BUILTIN,
        INSTR,
        FNAPP,
        FN,
        TUPLE
    };
    const Type type;
    explicit Expr(Type type) : type(type) {}
    virtual ~Expr() {}
};

struct String : Expr
{
    string x;
    explicit String(string x) : Expr(STRING), x(move(x)) {}
};

struct Number : Expr
{
    string x;
    explicit Number(string x) : Expr(NUMBER), x(move(x)) {}
};

struct Varname : Expr
{
    string x;
    explicit Varname(string x) : Expr(VARNAME), x(move(x)) {}
};

struct Builtin : Expr
{
    const ast::Builtin x;
    explicit Builtin(ast::Builtin x) : Expr(BUILTIN), x(x) {}
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

    Instr(Opcode opcode, const Expr* arg0) : Expr(INSTR), opcode(opcode), arg0(arg0)
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

struct Fnapp : Expr
{
    using Args = vector<FnArg>;
    const Expr* fn_to_apply;
    Args args;
    Fnapp(const Expr* fn_to_apply, Args args)
        : Expr(FNAPP), fn_to_apply(fn_to_apply), args(move(args))
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
    Pars pars, envpars;
    const Expr* body;
    Fn(Pars pars, Pars envpars, const Expr* body)
        : Expr(FN), pars(move(pars)), envpars(move(envpars)), body(body)
    {
    }
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

    Tuple() : Expr(TUPLE), has_names(false) {}
    explicit Tuple(vector<const Expr*> xs)
        : Expr(TUPLE), xs(vector_expr_to_vector_namedexpr(xs)), has_names(false)
    {
    }
    explicit Tuple(vector<NamedExpr> xs)
        : Expr(TUPLE),
          xs(move(xs)),
          has_names(std::any_of(BE(xs), [](auto& ne) { return !ne.n.empty(); }))
    {
    }
};

// ExprTypes must correspond to the order of Expr::Type
using ExprTypes = variant<String, Number, Varname, Builtin, Instr, Fnapp, Fn, Tuple>;
static_assert(std::is_same_v<typename std::variant_alternative_t<Expr::STRING, ExprTypes>, String>);
static_assert(std::is_same_v<typename std::variant_alternative_t<Expr::NUMBER, ExprTypes>, Number>);
static_assert(
    std::is_same_v<typename std::variant_alternative_t<Expr::VARNAME, ExprTypes>, Varname>);
static_assert(
    std::is_same_v<typename std::variant_alternative_t<Expr::BUILTIN, ExprTypes>, Builtin>);
static_assert(std::is_same_v<typename std::variant_alternative_t<Expr::INSTR, ExprTypes>, Instr>);
static_assert(std::is_same_v<typename std::variant_alternative_t<Expr::FNAPP, ExprTypes>, Fnapp>);
static_assert(std::is_same_v<typename std::variant_alternative_t<Expr::FN, ExprTypes>, Fn>);
static_assert(std::is_same_v<typename std::variant_alternative_t<Expr::TUPLE, ExprTypes>, Tuple>);

template <Expr::Type T>
auto cast(const Expr* e)
{
    using Target = typename std::variant_alternative_t<T, ExprTypes>;
    CHECK(e->type == T);
    return static_cast<const Target*>(e);
}

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
