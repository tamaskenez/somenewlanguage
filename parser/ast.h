#pragma once

#include <variant>

#include "util/ensure_char8_t.h"

namespace forrest {

using std::pair;
using std::u32string;
using std::variant;
using std::vector;

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

}  // namespace forrest
