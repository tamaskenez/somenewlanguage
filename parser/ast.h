#pragma once

#include <variant>

namespace forrest {

using std::variant;

namespace ast {

struct Expr;
using ExprRef = Expr*;
struct Exprs
{
    vector<ExprRef> xs;
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
    vector<ExprRef> args;
}

using Vec = variant<Exprs, AsciiStr, Str>;
using Expr = variant<Num, Vec, App, Char, Bool>;

}  // namespace ast

}  // namespace forrest
