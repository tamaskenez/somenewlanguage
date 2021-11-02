#pragma once

#include "common.h"

#include "context.h"
#include "number.h"
#include "term_forward.h"

namespace snl {
struct TypeAndAvailability
{
    TermPtr type;
    optional<term::Variable const*> comptime_parameter;
    TypeAndAvailability(TermPtr type, optional<term::Variable const*> comptime_parameter)
        : type(type), comptime_parameter(comptime_parameter)
    {}
    bool operator==(const TypeAndAvailability& y) const
    {
        return type == y.type && comptime_parameter == y.comptime_parameter;
    }
};
}  // namespace snl

namespace std {
template <>
struct hash<snl::TypeAndAvailability>
{
    std::size_t operator()(const snl::TypeAndAvailability& x) const noexcept
    {
        auto h = snl::hash_value(x.type);
        snl::hash_combine(h, x.comptime_parameter);
        return h;
    }
};
}  // namespace std

namespace snl {
namespace term {

enum class Tag
{
    // Core.
    Abstraction,
    LetIns,
    Application,
    Variable,

    // Literals.
    StringLiteral,
    NumericLiteral,

    // Values.
    UnitLikeValue,
    DeferredValue,
    ProductValue,

    // Fundamental types.
    SimpleTypeTerm,
    FunctionType,
    ProductType,
};

}

struct Term
{
    term::Tag const tag;
    explicit Term(term::Tag tag) : tag(tag) {}
};

struct ValueTerm : Term
{
    TermPtr const type;
    ValueTerm(term::Tag tag, TermPtr type) : Term(tag), type(type) {}
};

#define STATIC_TAG(X) static constexpr Tag s_tag = Tag::X

struct Store;

namespace term {

struct Variable : Term
{
    STATIC_TAG(Variable);

    bool comptime;  // The value we bind this variable to must be available at compile time.
    string name;    // For diagnostics.
    Variable(bool comptime, string&& name)
        : Term(Tag::Variable), comptime(comptime), name(move(name))
    {}
};
}  // namespace term
struct Parameter
{
    term::Variable const* variable;
    TermPtr expected_type;
    Parameter(term::Variable const* variable, TermPtr expected_type)
        : variable(variable), expected_type(expected_type)
    {}
    bool operator==(const Parameter& y) const
    {
        return variable == y.variable && expected_type == y.expected_type;
    }
};

struct BoundVariable
{
    term::Variable const* variable;
    TermPtr value;

    bool operator==(const BoundVariable& y) const
    {
        return variable == y.variable && value == y.value;
    }
};

}  // namespace snl

namespace std {

template <>
struct hash<snl::Parameter>
{
    std::size_t operator()(const snl::Parameter& x) const noexcept
    {
        auto h = snl::hash_value(x.variable);
        snl::hash_combine(h, x.expected_type);
        return h;
    }
};
template <>
struct hash<snl::BoundVariable>
{
    std::size_t operator()(const snl::BoundVariable& x) const noexcept
    {
        auto h = snl::hash_value(x.variable);
        snl::hash_combine(h, x.value);
        return h;
    }
};
}  // namespace std

namespace snl {
namespace term {

struct Abstraction : Term
{
    STATIC_TAG(Abstraction);

public:
    // The variables/terms here are in strict order: forall, bound, parameters, body.
    // The terms must have only free variables which has been introduced before the term, in the
    // order described above.
    unordered_set<Variable const*> forall_variables;  // A parameter is comptime if listed here.
    vector<BoundVariable> bound_variables;
    vector<Parameter> parameters;
    TermPtr body;

    static optional<Abstraction> MakeAbstraction(Store& store,
                                                 unordered_set<Variable const*>&& forall_variables,
                                                 vector<BoundVariable>&& bound_variables,
                                                 vector<Parameter>&& parameters,
                                                 TermPtr body);

private:
    Abstraction(unordered_set<Variable const*>&& forall_variables,
                vector<BoundVariable>&& bound_variables,
                vector<Parameter>&& parameters,
                TermPtr body)
        : Term(Tag::Abstraction),
          bound_variables(move(bound_variables)),
          parameters(move(parameters)),
          body(body)
    {}
};

// Simplified version of Abstraction: no parameters, no forall.
// bound_variables are the let-in clauses.
struct LetIns : Term
{
    STATIC_TAG(LetIns);
    vector<BoundVariable> bound_variables;
    TermPtr body;
    LetIns(vector<BoundVariable>&& bound_variables, TermPtr body)
        : Term(Tag::LetIns), bound_variables(move(bound_variables)), body(body)
    {}
};

struct Application : Term
{
    STATIC_TAG(Application);

    TermPtr function;
    vector<TermPtr> arguments;

    Application(TermPtr function, vector<TermPtr>&& arguments)
        : Term(Tag::Application), function(function), arguments(move(arguments))
    {}
};

struct StringLiteral : Term
{
    STATIC_TAG(StringLiteral);

    string value;
    explicit StringLiteral(string&& value) : Term(Tag::StringLiteral), value(move(value)) {}
};

struct NumericLiteral : Term
{
    STATIC_TAG(NumericLiteral);

    Number value;
    explicit NumericLiteral(Number&& value) : Term(Tag::NumericLiteral), value(move(value)) {}
};

struct TypeTerm : Term
{
    explicit TypeTerm(Tag tag) : Term(tag) {}
};

enum class SimpleType
{
    TypeOfTypes,
    Bottom,
    Unit,
    Top,
    UnresolvedAbstraction,
    TypeToBeInferred,  // Type that will only be available after the Abstraction gots a concrete
                       // application
    StringLiteral,
    NumericLiteral
};

struct SimpleTypeTerm : TypeTerm
{
    STATIC_TAG(SimpleTypeTerm);

    SimpleType simple_type;
    explicit SimpleTypeTerm(SimpleType simple_type)
        : TypeTerm(Tag::SimpleTypeTerm), simple_type(simple_type)
    {}
};

struct FunctionType : TypeTerm
{
    STATIC_TAG(FunctionType);

    unordered_set<Variable const*> forall_variables;
    vector<TypeAndAvailability> parameter_types;
    TermPtr result_type;
    FunctionType(unordered_set<Variable const*>&& forall_variables,
                 vector<TypeAndAvailability>&& parameter_types,
                 TermPtr result_type)
        : TypeTerm(Tag::FunctionType),
          forall_variables(move(forall_variables)),
          parameter_types(move(parameter_types)),
          result_type(result_type)
    {}
};

struct ProductType : TypeTerm
{
    STATIC_TAG(ProductType);

    unordered_map<string, TermPtr> members;

    explicit ProductType(unordered_map<string, TermPtr>&& members)
        : TypeTerm(Tag::ProductType), members(move(members))
    {}
};

// An unspecified value which will be resolved later, comptime or runtime.
struct DeferredValue : ValueTerm
{
    STATIC_TAG(DeferredValue);
    enum class Availability
    {
        Runtime,  // We will have a concrete value here only at runtime.
        Comptime  // This value has to be resolved in comptime.
    } availability;
    DeferredValue(TermPtr type, Availability availability)
        : ValueTerm(Tag::DeferredValue, type), availability(availability)
    {}
};

// Value of a type which is isomorph to Unit (single inhabitant)
struct UnitLikeValue : ValueTerm
{
    STATIC_TAG(UnitLikeValue);
    explicit UnitLikeValue(TermPtr type) : ValueTerm(Tag::UnitLikeValue, type) {}
};

struct ProductValue : ValueTerm
{
    STATIC_TAG(ProductValue);
    unordered_map<string, TermPtr> values;
    ProductValue(TermPtr type, unordered_map<string, TermPtr>&& values)
        : ValueTerm(Tag::ProductValue, type), values(move(values))
    {}
};

}  // namespace term

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
// bool IsTypeInNormalForm(TermPtr p);

struct TermHash
{
    std::size_t operator()(TermPtr t) const noexcept;
};

struct TermEqual
{
    bool operator()(TermPtr x, TermPtr y) const noexcept;
};

}  // namespace snl
