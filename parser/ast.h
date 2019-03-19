#pragma once

#include <variant>

namespace forrest {

using std::variant;

namespace ast {

struct Expr;

struct Exprs
{
    vector<Expr> xs;
};

struct Char
{
    uint32_t x;
};

struct AsciiStr
{
    string xs;
};

struct Str
{
    string xs;
};

struct Num
{
    string x;
};

struct Bool
{
    bool x;
};

struct App
{
    string sym;
    vector<Expr> args;
}

using Vec = variant<Exprs, AsciiStr, Str>;
using Expr = variant<Num, Vec, App, Char, Bool>;

}  // namespace ast

}  // namespace forrest
