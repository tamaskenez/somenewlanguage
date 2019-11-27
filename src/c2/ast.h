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
    bool quoted;

    Token(string x, bool quoted) : x(move(x)), quoted(quoted) {}
};

struct List
{
    vector<Expr*> xs;

    List(vector<Expr*> xs) : xs(move(xs)) {}
};

}  // namespace ast

class Ast
{
    deque<ast::Expr> exprs;

public:
    ast::Expr* new_list(vector<ast::Expr*> xs)
    {
        return &exprs.emplace_back(in_place_type<ast::List>, move(xs));
    }
    ast::Expr* new_token(string x, bool quoted)
    {
        return &exprs.emplace_back(in_place_type<ast::Token>, move(x), quoted);
    }
};

void dump(ast::Expr* expr);

}  // namespace forrest
