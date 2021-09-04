#pragma once

#include "common.h"

namespace snl {
namespace term {

struct Term;
using TermPtr = Term const*;

enum class Tag
{
    Abstraction,
    Application,
    Variable,
    StringLiteral,

    TypeOfTypes,
    UnitType,
    BottomType,
    TopType,
    StringLiteralType,
};

struct Term
{
    Tag const tag;
    TermPtr const type;  // Type of the term, also a term.

    Term(Tag tag, TermPtr type) : tag(tag), type(type) {}

protected:
    ~Term() = default;
};

struct Parameter
{
    string name;
    optional<TermPtr> type_annotation;  // Todo: Should be an expression, too.
    bool operator==(const Parameter& y) const
    {
        return name == y.name && type_annotation == y.type_annotation;
    }
};

}  // namespace term
}  // namespace snl

namespace std {
template <>
struct hash<snl::term::Parameter>
{
    std::size_t operator()(const snl::term::Parameter& x) const noexcept
    {
        auto h = std::hash<string>{}(x.name);
        if (x.type_annotation.has_value()) {
            h = (h << 1) ^ std::hash<snl::term::TermPtr>{}(*x.type_annotation);
        }
        return h;
    }
};
}  // namespace std

namespace snl {
namespace term {

struct Abstraction : Term
{
    vector<Parameter> parameters;
    TermPtr body;

    Abstraction(TermPtr type, vector<Parameter>&& parameters, TermPtr body)
        : Term(Tag::Abstraction, type), parameters(move(parameters)), body(body)
    {}
};

struct Application : Term
{
    TermPtr function;
    vector<TermPtr> arguments;

    Application(TermPtr type, TermPtr function, vector<TermPtr>&& arguments)
        : Term(Tag::Application, type), function(function), arguments(move(arguments))
    {}
};

struct Variable : Term
{
    string name;
    Variable(TermPtr type, string&& name) : Term(Tag::Variable, type), name(move(name)) {}
};

struct TypeTerm : Term
{
    explicit TypeTerm(Tag tag);
    TypeTerm(Tag tag, TermPtr canonicalTypeOfTypesPtr) : Term(tag, canonicalTypeOfTypesPtr) {}
};

struct TypeOfTypes : TypeTerm
{
    TypeOfTypes() : TypeTerm(Tag::TypeOfTypes, this) {}
};

struct BottomType : TypeTerm
{
    BottomType() : TypeTerm(Tag::BottomType) {}
};

struct UnitType : TypeTerm
{
    UnitType() : TypeTerm(Tag::UnitType) {}
};

struct TopType : TypeTerm
{
    TopType() : TypeTerm(Tag::TopType) {}
};

struct StringLiteralType : TypeTerm
{
    int size;
    explicit StringLiteralType(int size) : TypeTerm(Tag::StringLiteralType), size(size) {}
};

struct TermHash
{
    std::size_t operator()(TermPtr t) const noexcept;
};

struct TermEqual
{
    bool operator()(TermPtr x, TermPtr y) const noexcept;
};

struct Store
{
    Store();
    ~Store();

    TermPtr MakeCanonical(Term&& term);
    bool IsCanonical(TermPtr x) const;

    unordered_set<TermPtr, TermHash, TermEqual> canonical_terms;

    TermPtr const type_of_types;
    TermPtr const bottom_type;
    TermPtr const unit_type;
    TermPtr const top_type;

    static TypeOfTypes
        s_type_of_types_canonical_instance;  // Special handling because of recursive nature.
private:
    TermPtr MoveToHeap(Term&& t);
};

struct StringLiteral : Term
{
    string value;
    StringLiteral(Store& store, string&& value)
        : Term(Tag::Variable, store.MakeCanonical(StringLiteralType(value.size()))),
          value(move(value))
    {}
};

}  // namespace term
}  // namespace snl
