#pragma once

#include <deque>
#include <variant>
#include <vector>

#include "util/ensure_char8_t.h"

#include "common.h"

namespace forrest {

using std::deque;
using std::pair;
using std::string;
using std::u32string;
using std::variant;
using std::vector;

using SymbolName = u8string;

struct Symbol
{};

using SymbolRef = pair<const u8string, Symbol>*;

// Vectors
struct VecNode;
struct StrNode;

// Scalars
struct SymLeaf;
struct NumLeaf;
struct CharLeaf;

using Expr = variant<VecNode, StrNode, SymLeaf, NumLeaf, CharLeaf>;

using ExprRef = Expr*;

struct VecNode
{
    bool apply;
    vector<ExprRef> xs;
    explicit VecNode(bool apply) : apply(apply) {}
};

struct StrNode
{
    u8string xs;
    explicit StrNode(u8string xs) : xs(move(xs)) {}
};

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
};

void dump(const Ast& ast);

}  // namespace forrest
