#pragma once

#include <deque>
#include <string>
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

using SymbolRef = pair<const string, Symbol>*;

// Vectors
struct VecNode;

struct StrNode
{
    string xs;
    explicit StrNode(string xs) : xs(move(xs)) {}
};

// Scalars

struct SymLeaf
{
    SymbolRef sym;
    explicit SymLeaf(SymbolRef sym) : sym(sym) {}
};

struct NumLeaf
{
    string x;
    explicit NumLeaf(string x) : x(x) {}
};

struct CharLeaf
{
    char32_t x;
    explicit CharLeaf(char32_t x) : x(x) {}
};

struct VoidLeaf
{};

using Expr = variant<VecNode, StrNode, SymLeaf, NumLeaf, CharLeaf, VoidLeaf>;

using ExprRef = Expr*;

struct VecNode
{
    bool apply;
    vector<ExprRef> xs;
    explicit VecNode(bool apply) : apply(apply) {}
    VecNode(bool apply, vector<ExprRef> xs) : apply(apply), xs(move(xs)) {}
};

struct Ast
{
    deque<Expr> storage;
    vector<ExprRef> top_level_exprs;
    StableHashMap<SymbolName, Symbol> symbols;

    SymbolRef get_or_create_symbolref(SymbolName x)
    {
        auto itb = symbols.try_emplace(x);
        return &*(itb.first);
    }

    Ast();

    ExprRef empty_vecnode() const { return _empty_vecnode; }
    ExprRef voidleaf() const { return _voidleaf; }

private:
    ExprRef _empty_vecnode;
    ExprRef _voidleaf;
};

void dump(const Ast& ast);
void dump(ExprRef ast);

}  // namespace forrest
