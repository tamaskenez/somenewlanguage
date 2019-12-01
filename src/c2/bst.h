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

struct Application;

using Expr = variant<String, Number, Application, Varname>;

struct Application
{
    const Expr* head;
    vector<Expr*> args;
    vector<Expr*> envargs;
    explicit Application(const Expr* head) : head(head) {}
    inline Application(const Expr* head, vector<Expr*> args);
    inline Application(const Expr* head, vector<Expr*> args, vector<Expr*> envargs);
};

Application::Application(const Expr* head, vector<Expr*> args, vector<Expr*> envargs)
    : head(head), args(move(args)), envargs(move(envargs))
{
}

Application::Application(const Expr* head, vector<Expr*> args) : head(head), args(move(args)) {}

const auto VARNAME_TUPLE = Expr{in_place_type<Varname>, "tuple"};
const auto VARNAME_VECTOR = Expr{in_place_type<Varname>, "vector"};
const auto VARNAME_FN = Expr{in_place_type<Varname>, "fn"};
const auto VARNAME_DATA = Expr{in_place_type<Varname>, "data"};
const auto VARNAME_DEF = Expr{in_place_type<Varname>, "def"};

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
