#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "common.h"

namespace forrest {

using std::deque;
using std::pair;
using std::string;
using std::variant;
using std::vector;

// Vectors
struct TupleNode;

struct StrNode
{
    string xs;
    explicit StrNode(string xs) : xs(move(xs)) {}
};

// Scalars

struct SymLeaf
{
    string name;
    explicit SymLeaf(string name) : name(move(name)) {}
};

struct NumLeaf
{
    string x;
    explicit NumLeaf(string x) : x(move(x)) {}
};

struct CharLeaf
{
    char32_t x;
    explicit CharLeaf(char32_t x) : x(x) {}
};

struct ApplyNode
{
    TupleNode* tuple;
    explicit ApplyNode(TupleNode* tuple) : tuple(tuple) {}
};

struct QuoteNode;

using Expr = variant<TupleNode, StrNode, SymLeaf, NumLeaf, CharLeaf, ApplyNode, QuoteNode>;

using ExprPtr =
    variant<TupleNode*, StrNode*, SymLeaf*, NumLeaf*, CharLeaf*, ApplyNode*, QuoteNode*>;

struct QuoteNode
{
    ExprPtr expr;
    explicit QuoteNode(ExprPtr expr) : expr(expr) {}
};

struct TupleNode
{
    vector<ExprPtr> xs;
    TupleNode() = default;
    template <class First, class Last>
    TupleNode(First first, Last last) : xs(first, last)
    {}
};

void dump(ExprPtr expr);

}  // namespace forrest
