#pragma once

#include "ul/usual.h"

#include "ast.h"

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

struct Tuple;
struct Vector;
struct Fn;
struct Data;
struct Application;

using Expr = variant<Tuple, Vector, String, Number, Fn, Data, Application, Varname>;

struct Fn
{
    vector<string> pars, envpars;
    Expr* body;
    Fn(vector<string> pars, vector<string> envpars, Expr* body)
        : pars(move(pars)), envpars(move(envpars)), body(body)
    {
    }
};

struct Data
{
    string name;
    Expr* def;
    Data(string name, Expr* def) : name(move(name)), def(def) {}
};

struct Application
{
    Expr* head;
    vector<Expr*> args;
    vector<Expr*> envargs;
    inline Application(Expr* head, vector<Expr*> args, vector<Expr*> envargs);
};

struct Tuple
{
    vector<Expr*> xs;
    inline explicit Tuple(vector<Expr*> xs);
};

struct Vector : Tuple
{
    explicit Vector(vector<Expr*> xs) : Tuple(move(xs)) {}
};

Tuple::Tuple(vector<Expr*> xs) : xs(move(xs)) {}

Application::Application(Expr* head, vector<Expr*> args, vector<Expr*> envargs)
    : head(head), args(move(args)), envargs(move(envargs))
{
}

struct VarContent
{};
struct Env
{
    unordered_map<string, VarContent> local_map;
    Env* parent = nullptr;

    maybe<VarContent*> lookup(const string& key)
    {
        auto it = local_map.find(key);
        if (it != local_map.end()) {
            return &(it->second);
        }
        if (parent) {
            return parent->lookup(key);
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

}  // namespace forrest
