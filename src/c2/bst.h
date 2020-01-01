#pragma once

#include <typeindex>

#include "ast.h"
#include "ast_syntax.h"
#include "ul/usual.h"

namespace forrest {

using namespace ul;

struct StringTree
{
    string text;
    vector<StringTree*> children;

    explicit StringTree(string s) : text(move(s)) {}
    explicit StringTree(vector<StringTree*> children) : children(move(children)) {}
    StringTree(string s, vector<StringTree*> children) : text(move(s)), children(move(children)) {}
    bool only_leaf_children() const
    {
        for (auto c : children) {
            if (!c->children.empty()) {
                return false;
            }
        }
        return true;
    }
};

string to_string(StringTree* st, bool oneline = true, int indent = 0);

namespace bst {

struct Fn;

enum Type
{
    tString,
    tNumber,
    tTuple,
    tVariable,
    tFnapp,
    tFn,
    tDef,
    tLet,
    tToplevelVariableName
};

struct Expr
{
    Type type;
    explicit Expr(Type type) : type(type) {}
    virtual ~Expr() {}
    virtual StringTree* to_stringtree() const = 0;
};

struct String : Expr
{
    string x;
    explicit String(string x) : Expr(tString), x(move(x)) {}
    virtual StringTree* to_stringtree() const { return new StringTree("\"" + x + "\""); }
};

struct Number : Expr
{
    string x;
    explicit Number(string x) : Expr(tNumber), x(move(x)) {}
    virtual StringTree* to_stringtree() const { return new StringTree("#" + x); }
};

struct Variable : Expr
{
    string name;
    explicit Variable(string name) : Expr(tVariable), name(move(name)) {}
    virtual StringTree* to_stringtree() const
    {
        return new StringTree(name.empty() ? string("\"\"") : name);
    }
};

/*
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
*/

struct Fnapp : Expr
{
    const Expr* const fn_to_apply;
    const vector<const Expr*> args;
    Fnapp(const Expr* fn_to_apply, vector<const Expr*> args)
        : Expr(tFnapp), fn_to_apply(fn_to_apply), args(move(args))
    {
        CHECK(!this->args.empty());
    }
    virtual StringTree* to_stringtree() const
    {
        vector<StringTree*> children = {fn_to_apply->to_stringtree()};
        for (auto a : args) {
            children.emplace_back(a->to_stringtree());
        }
        return new StringTree("<fnapp>", move(children));
    }
};

struct LexicalScope
{
    const maybe<const LexicalScope*> enclosing;
    const vector<const Variable*> vs;
    LexicalScope() = default;
    LexicalScope(const LexicalScope* enclosing, vector<const Variable*> vs)
        : enclosing(enclosing), vs(move(vs))
    {
        CHECK(enclosing);
    }
    maybe<const Variable*> try_resolve_variable_name(const string& s) const
    {
        for (auto v : vs) {
            if (v->name == s) {
                return v;
            }
        }
        if (enclosing) {
            return (*enclosing)->try_resolve_variable_name(s);
        }
        return {};
    }
};

struct ToplevelVariableName : Expr
{
    const string name;
    ToplevelVariableName(string name) : Expr(tToplevelVariableName), name(move(name)) {}
    virtual StringTree* to_stringtree() const { return new StringTree("<tlvar> " + name); }
};

using FnPar = const Variable*;

struct Fn : Expr
{
    using Pars = vector<FnPar>;
    Pars pars;
    const Expr* body;
    Fn(Pars pars, const Expr* body) : Expr(tFn), pars(move(pars)), body(body)
    {
        CHECK(!this->pars.empty());
    }
    virtual StringTree* to_stringtree() const
    {
        vector<StringTree*> stpars, children;
        for (auto& p : pars) {
            stpars.emplace_back(new StringTree(p->name));
        }
        children = {new StringTree(move(stpars)), body->to_stringtree()};
        return new StringTree("<fn>", children);
    }
};

struct Def : Expr
{
    string name;
    const Expr* e;
    Def(string name, const Expr* e) : Expr(tDef), name(move(name)), e(e) {}
    virtual StringTree* to_stringtree() const
    {
        return new StringTree("<def> " + (name.empty() ? string("\"\"") : name),
                              vector<StringTree*>{e->to_stringtree()});
    }
};

struct Let : Expr
{
    string name;
    const Expr* value;
    const Expr* body;
    Let(string name, const Expr* value, const Expr* body)
        : Expr(tLet), name(move(name)), value(value), body(body)
    {
    }
    virtual StringTree* to_stringtree() const
    {
        return new StringTree("<let> " + (name.empty() ? string("\"\"") : name),
                              vector<StringTree*>{value->to_stringtree(), body->to_stringtree()});
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
    const Expr* operator[](int i) const { return xs[i].x; }
    int size() const { return ~xs; }
    virtual StringTree* to_stringtree() const
    {
        vector<StringTree*> children;
        if (has_names) {
            for (auto x : xs) {
                children.emplace_back(
                    new StringTree(x.n, vector<StringTree*>{x.x->to_stringtree()}));
            }
        } else {
            for (auto x : xs) {
                children.emplace_back(x.x->to_stringtree());
            }
        }
        return new StringTree("<tuple>", children);
    }
};

/*
struct Builtin : Expr
{
    const ast::Builtin head;
    const Tuple* xs;
    Builtin(ast::Builtin head, const Tuple* xs) : Expr(tBuiltin), head(head), xs(xs) {}
};
*/

const auto EXPR_EMPTY_TUPLE = Tuple{};

/*
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
        local_chain;  // Subexpressions, local nonescaping lambdas inherit parent local scope
chain. const TupleChain dynamic_chain;  // Callees inherit callers' dynamic scope chain.
    Env(const Tuple* top_level_lexical,
        const vector<const Tuple*> opened_lexicals,
        TupleChain local_chain,
        TupleChain dynamic_chain)
        : top_level_lexical(top_level_lexical),
          opened_lexicals(opened_lexicals),
          local_chain(local_chain),
          dynamic_chain(dynamic_chain)
    {}
};
*/

template <class T>
const T* cast(const Expr* e)
{
    const T* t = dynamic_cast<const T*>(e);
    CHECK(t);
    return t;
}

template <class T>
T* cast(Expr* e)
{
    T* t = dynamic_cast<T*>(e);
    CHECK(t);
    return t;
}

template <class T>
const T* try_cast(const Expr* e)
{
    return dynamic_cast<const T*>(e);
}

template <class T>
T* try_cast(Expr* e)
{
    return dynamic_cast<T*>(e);
}

}  // namespace bst

/*
struct Bst
{
    const bst::Tuple* top_level_lexical_scope;
    const bst::Tuple* top_level_dynamic_scope;
    Bst(const bst::Tuple* top_level_lexical_scope, const bst::Tuple* top_level_dynamic_scope)
        : top_level_lexical_scope(top_level_lexical_scope),
          top_level_dynamic_scope(top_level_dynamic_scope)
    {}
};

 */
const bst::Expr* process_ast(ast::Expr* e, const bst::LexicalScope* ls);
// void dump(const bst::Expr* expr);
// void dump_dfs(const bst::Expr* expr);
maybe<const bst::Expr*> lookup_by_name(const bst::Tuple* t, const string& n);

}  // namespace forrest
