#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "common.h"

namespace forrest {

using std::deque;
using std::holds_alternative;
using std::pair;
using std::string;
using std::variant;
using std::vector;

struct CharLeaf;
struct NumLeaf;
struct SymLeaf;
struct StrNode;
struct TupleNode;
struct ApplyNode;
struct QuoteNode;
namespace tag {
struct Char
{
    using Ptr = CharLeaf*;
};
struct Num
{
    using Ptr = NumLeaf*;
};
struct Sym
{
    using Ptr = SymLeaf*;
};
struct Str
{
    using Ptr = StrNode*;
};
struct Tuple
{
    using Ptr = TupleNode*;
};
struct Apply
{
    using Ptr = ApplyNode*;
};
struct Quote
{
    using Ptr = QuoteNode*;
};
};  // namespace tag

using Tag = variant<tag::Char, tag::Num, tag::Sym, tag::Str, tag::Tuple, tag::Apply, tag::Quote>;

using NodePV = variant<CharLeaf*, NumLeaf*, SymLeaf*, StrNode*, TupleNode*, ApplyNode*, QuoteNode*>;

struct Node
{
    const Tag tag;
    Node(Tag tag) : tag(tag) {}
    ~Node() = default;
    template <class Tag>
    auto try_cast() const
    {
        return holds_alternative<Tag>(tag) ? (typename Tag::Ptr)this : nullptr;
    }
    virtual NodePV thisv() = 0;
};

struct StrNode : Node
{
    const string xs;
    explicit StrNode(string xs) : Node(tag::Str{}), xs(move(xs)) {}
    NodePV thisv() override { return this; }
};

struct SymLeaf : Node
{
    const string name;
    explicit SymLeaf(string name) : Node(tag::Sym{}), name(move(name)) {}
    NodePV thisv() override { return this; }
};

struct NumLeaf : Node
{
    const string x;
    explicit NumLeaf(string x) : Node(tag::Num{}), x(move(x)) {}
    NodePV thisv() override { return this; }
};

struct CharLeaf : Node
{
    const char32_t x;
    explicit CharLeaf(char32_t x) : Node(tag::Char{}), x(x) {}
    NodePV thisv() override { return this; }
};

struct TupleNode : Node
{
    const vector<Node*> xs;
    TupleNode() : Node(tag::Tuple{}) {}
    template <class First, class Last>
    TupleNode(First first, Last last) : Node(tag::Tuple{}), xs(first, last)
    {}
    NodePV thisv() override { return this; }
};

struct ApplyNode : Node
{
    Node* const lambda;
    TupleNode* const args;
    ApplyNode(Node* lambda, TupleNode* args) : Node(tag::Apply{}), lambda(lambda), args(args) {}
    NodePV thisv() override { return this; }
};

struct QuoteNode : Node
{
    Node* const expr;
    explicit QuoteNode(Node* expr) : Node(tag::Quote{}), expr(expr) {}
    NodePV thisv() override { return this; }
};

void dump(Node* expr);

}  // namespace forrest
