#pragma once

#include <deque>
#include <string>
#include <variant>
#include <vector>

namespace forrest {

using std::deque;
using std::in_place_type;
using std::string;
using std::variant;
using std::vector;

namespace ast {

struct List;
struct Token;

using Expr = variant<List, Token>;

struct Token
{
    string x;
    enum Kind
    {
        STRING,
        QUOTED_STRING,
        NUMBER
    } kind;

    Token(string x, Token::Kind kind) : x(move(x)), kind(kind) {}
};

struct List
{
    bool fnapp;
    vector<Expr*> xs;

    List(bool fnapp, vector<Expr*> xs) : fnapp(fnapp), xs(move(xs)) {}
};

}  // namespace ast

class Ast
{
    deque<ast::Expr> exprs;

public:
    ast::Expr* new_list(bool fnapp, vector<ast::Expr*> xs)
    {
        return &exprs.emplace_back(in_place_type<ast::List>, fnapp, move(xs));
    }
    ast::Expr* new_token(string x, ast::Token::Kind kind)
    {
        return &exprs.emplace_back(in_place_type<ast::Token>, move(x), kind);
    }
};

void dump(ast::Expr* expr);
bool is_env_args_separator(ast::Expr* e);

}  // namespace forrest
