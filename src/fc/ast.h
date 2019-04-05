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

using SymbolName = string;

struct Symbol
{};

using NameSymbolRef = pair<const string, Symbol>*;

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
    NameSymbolRef sym;
    explicit SymLeaf(NameSymbolRef sym) : sym(sym) {}
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

struct VoidLeaf
{};

struct ApplyNode
{
    TupleNode* tuple_ref;
    explicit ApplyNode(TupleNode* tuple_ref) : tuple_ref(tuple_ref) {}
};

struct QuoteNode;

using Expr =
    variant<TupleNode, StrNode, SymLeaf, NumLeaf, CharLeaf, VoidLeaf, ApplyNode, QuoteNode>;

using ExprRef = Expr*;

struct QuoteNode
{
    Expr* expr_ref;
};

struct TupleNode
{
    vector<ExprRef> xs;
    TupleNode() = default;
    explicit TupleNode(vector<ExprRef> xs) : xs(move(xs)) {}
};

struct Ast
{
    deque<Expr> storage;
    vector<ExprRef> top_level_exprs;
    StableHashMap<SymbolName, Symbol> symbols;

    NameSymbolRef get_or_create_symbolref(SymbolName x)
    {
        auto itb = symbols.try_emplace(x);
        return &*(itb.first);
    }

    Ast();

    ExprRef empty_tuplenode() const { return _empty_tuplenode; }
    ExprRef voidleaf() const { return _voidleaf; }

private:
    ExprRef _empty_tuplenode;
    ExprRef _voidleaf;
};

void dump(const Ast& ast);
void dump(ExprRef ast);

}  // namespace forrest
