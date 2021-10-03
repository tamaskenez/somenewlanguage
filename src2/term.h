#pragma once

#include "common.h"

#include "bound_variables.h"
#include "term_forward.h"

namespace snl {
namespace term {

enum class Tag
{
    // Core.
    Abstraction,
    Application,
    Variable,
    Projection,

    // Literals.
    StringLiteral,
    NumericLiteral,

    // Values.
    RuntimeValue,
    ProductValue,

    // Core types.
    TypeOfTypes,
    UnitType,
    BottomType,
    TopType,

    // Fundamental types.
    FunctionType,
    ProductType,

    // Literal types.
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

#define STATIC_TAG(X) static constexpr Tag s_tag = Tag::X

struct Variable : Term
{
    STATIC_TAG(Variable);

    string name;  // For diagnostics.
    Variable(TermPtr type, string&& name) : Term(Tag::Variable, type), name(move(name)) {}
};

struct Parameter
{
    Variable const* variable;
    bool operator==(const Parameter& y) const { return variable == y.variable; }
};

struct BoundVariable
{
    Variable const* variable;
    TermPtr value;

    bool operator==(const BoundVariable& y) const
    {
        return variable == y.variable && value == y.value;
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
        auto h = snl::hash_value(x.variable);
        return h;
    }
};
template <>
struct hash<snl::term::BoundVariable>
{
    std::size_t operator()(const snl::term::BoundVariable& x) const noexcept
    {
        auto h = snl::hash_value(x.variable);
        snl::hash_combine(h, snl::hash_value(x.value));
        return h;
    }
};
}  // namespace std

namespace snl {
namespace term {

struct Store;

// Introduces a number of free variables for the body. Part of these variables cab be bound by
// supplying the abstraction to an application. Others can be bound here.
// The type must reflect the number of arguments this term expects.
// If the number of arguments is zero, this is a value term, no need to put into application.
struct Abstraction : Term
{
    STATIC_TAG(Abstraction);

private:
    static TermPtr TypeFromParametersAndResult(Store& store,
                                               const vector<Parameter>& parameters,
                                               TermPtr result_type);

public:
    vector<Parameter> parameters;
    vector<BoundVariable> bound_variables;
    TermPtr body;

    Abstraction(Store& store,
                vector<Parameter>&& parameters,
                vector<BoundVariable>&& bound_variables,
                TermPtr body)
        : Term(Tag::Abstraction, TypeFromParametersAndResult(store, parameters, body->type)),
          parameters(move(parameters)),
          bound_variables(bound_variables),
          body(body)
    {}
    TermPtr ResultType() const;
};

struct Application : Term
{
    STATIC_TAG(Application);

    TermPtr function;
    vector<TermPtr> arguments;

    Application(TermPtr type, TermPtr function, vector<TermPtr>&& arguments)
        : Term(Tag::Application, type), function(function), arguments(move(arguments))
    {}
};

struct Projection : Term
{
    STATIC_TAG(Projection);

    TermPtr domain;
    string codomain;
    Projection(TermPtr type, TermPtr domain, string&& codomain)
        : Term(Tag::Projection, type), domain(domain), codomain(move(codomain))
    {}
};

struct StringLiteral : Term
{
    STATIC_TAG(StringLiteral);

    string value;
    StringLiteral(Store& store, string&& value);
};

struct NumericLiteral : Term
{
    STATIC_TAG(NumericLiteral);

    string value;
    NumericLiteral(Store& store, string&& value);
};

struct TypeTerm : Term
{
    explicit TypeTerm(Tag tag);
    TypeTerm(Tag tag, TermPtr canonicalTypeOfTypesPtr) : Term(tag, canonicalTypeOfTypesPtr) {}
};

struct TypeOfTypes : TypeTerm
{
    STATIC_TAG(TypeOfTypes);

    TypeOfTypes() : TypeTerm(Tag::TypeOfTypes, this) {}
};

struct BottomType : TypeTerm
{
    STATIC_TAG(BottomType);

    BottomType() : TypeTerm(Tag::BottomType) {}
};

struct UnitType : TypeTerm
{
    STATIC_TAG(UnitType);

    UnitType() : TypeTerm(Tag::UnitType) {}
};

struct TopType : TypeTerm
{
    STATIC_TAG(TopType);

    TopType() : TypeTerm(Tag::TopType) {}
};

struct FunctionType : TypeTerm
{
    STATIC_TAG(FunctionType);

    vector<TermPtr> operand_types;  // A -> B -> C -> ... -> Result
    FunctionType(vector<TermPtr>&& operand_types)
        : TypeTerm(Tag::FunctionType), operand_types(move(operand_types))
    {}
};

struct TaggedType
{
    string tag;
    TermPtr type;
    bool operator==(const TaggedType& x) const { return tag == x.tag && type == x.type; }
};

}  // namespace term
}  // namespace snl

namespace std {
template <>
struct hash<snl::term::TaggedType>
{
    std::size_t operator()(const snl::term::TaggedType& x) const noexcept
    {
        auto h = snl::hash_value(x.tag);
        snl::hash_combine(h, snl::hash_value(x.type));
        return h;
    }
};
}  // namespace std

namespace snl {
namespace term {

struct ProductType : TypeTerm
{
    STATIC_TAG(ProductType);

    vector<TaggedType> members;
    explicit ProductType(vector<TaggedType>&& members)
        : TypeTerm(Tag::ProductType), members(move(members))
    {}
};

struct RuntimeValue : Term
{
    STATIC_TAG(RuntimeValue);
    RuntimeValue(TermPtr type) : Term(Tag::RuntimeValue, type) {}
};

struct ProductValue : Term
{
    STATIC_TAG(ProductValue);
    vector<TermPtr> values;
    ProductValue(TermPtr type, vector<TermPtr>&& values)
        : Term(Tag::ProductValue, type), values(move(values))
    {}
};

struct StringLiteralType : TypeTerm
{
    STATIC_TAG(StringLiteralType);

    StringLiteralType() : TypeTerm(Tag::StringLiteralType) {}
};

struct NumericLiteralType : TypeTerm
{
    STATIC_TAG(NumericLiteralType);

    NumericLiteralType() : TypeTerm(Tag::NumericLiteralType) {}
};

template <class T>
T const* term_cast(TermPtr p)
{
    assert(p->tag == T::s_tag);
    return static_cast<T const*>(p);
}

/*
// Visitor should return true to abort the traversal.
void DepthFirstTraversal(TermPtr p, std::function<bool(TermPtr)>& f);
*/
bool IsTypeInNormalForm(TermPtr p);

struct TermHash
{
    std::size_t operator()(TermPtr t) const noexcept;
};

struct TermEqual
{
    bool operator()(TermPtr x, TermPtr y) const noexcept;
};

// Store.

struct Store
{
    Store();
    ~Store();

    TermPtr MakeCanonical(Term&& term);
    bool IsCanonical(TermPtr x) const;
    Variable const* MakeNewTypeVariable();  // Make a new Variable with a type of TypeOfTypes.
    Variable const* MakeNewVariable(
        string&& name,
        TermPtr type = nullptr);  // Make a new Variable with specified type or
                                  // a type of a new type variable.

    TermPtr const type_of_types;

    unordered_set<TermPtr, TermHash, TermEqual> canonical_terms;

    TermPtr const bottom_type;
    TermPtr const unit_type;
    TermPtr const top_type;
    TermPtr const string_literal_type;
    TermPtr const numeric_literal_type;

    unordered_map<TermPtr, TermPtr> memoized_comptime_applications;

    static TypeOfTypes
        s_type_of_types_canonical_instance;  // Special handling because of recursive nature.
    static string const s_ignored_name;

private:
    TermPtr MoveToHeap(Term&& t);
    int next_generated_variable_id = 1;
};

}  // namespace term
}  // namespace snl
