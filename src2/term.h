#pragma once

#include "common.h"

namespace snl {
namespace term {

struct Term;
using TermPtr = Term const*;

enum class Tag
{
    // Core.
    Abstraction,
    Application,
    Variable,
    Projection,

    // Literal.
    StringLiteral,
    NumericLiteral,

    // Will be simplified.
    LetIn,
    SequenceYieldLast,

    // Core types.
    TypeOfTypes,
    UnitType,
    BottomType,
    TopType,
    InferredType,

    // Type functions.
    FunctionType,

    // Special types.
    StringLiteralType,
    NumericLiteralType
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

struct Projection : Term
{
    TermPtr domain;
    string codomain;
    Projection(TermPtr type, TermPtr domain, string&& codomain)
        : Term(Tag::Projection, type), domain(domain), codomain(move(codomain))
    {}
};

struct Store;
struct StringLiteral : Term
{
    string value;
    StringLiteral(Store& store, string&& value);
};

struct NumericLiteral : Term
{
    string value;
    NumericLiteral(Store& store, string&& value);
};

struct LetIn : Term
{
    string variable_name;
    TermPtr initializer, body;
    LetIn(TermPtr type, string&& variable_name, TermPtr initializer, TermPtr body)
        : Term(Tag::LetIn, type),
          variable_name(move(variable_name)),
          initializer(initializer),
          body(body)
    {}
};

struct SequenceYieldLast : Term
{
    vector<TermPtr> terms;
    SequenceYieldLast(Store& store, vector<TermPtr>&& terms);
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

// Denotes a type which will be inferred in a particular context. The context would refer to this
// type (variable) by the `id`.
struct InferredType : TypeTerm
{
    int id;
    explicit InferredType(int id) : TypeTerm(Tag::InferredType), id(id) {}
};

struct FunctionType : TypeTerm
{
    vector<TermPtr> terms;  // A -> B -> C -> ... -> Result
    FunctionType(vector<TermPtr>&& terms) : TypeTerm(Tag::FunctionType), terms(move(terms)) {}
};

struct StringLiteralType : TypeTerm
{
    StringLiteralType() : TypeTerm(Tag::StringLiteralType) {}
};

struct NumericLiteralType : TypeTerm
{
    NumericLiteralType() : TypeTerm(Tag::NumericLiteralType) {}
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
    TermPtr MakeInferredTypeTerm();  // Make a new InferredType term with the next available id.

    unordered_set<TermPtr, TermHash, TermEqual> canonical_terms;

    TermPtr const type_of_types;
    TermPtr const bottom_type;
    TermPtr const unit_type;
    TermPtr const top_type;
    TermPtr const string_literal_type;
    TermPtr const numeric_literal_type;

    static TypeOfTypes
        s_type_of_types_canonical_instance;  // Special handling because of recursive nature.
private:
    TermPtr MoveToHeap(Term&& t);
    int next_inferred_type_id = 1;
};

}  // namespace term
}  // namespace snl
