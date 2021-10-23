#pragma once

#include "common.h"

#include "context.h"
#include "term_forward.h"

namespace snl {
struct TypeAndAvailability
{
    TermPtr type;
    bool comptime;
    TypeAndAvailability(TermPtr type, bool comptime) : type(type), comptime(comptime) {}
    bool operator==(const TypeAndAvailability& y) const
    {
        return type == y.type && comptime == y.comptime;
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
        snl::hash_combine(h, x.comptime);
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
    ForAll,
    Application,
    Variable,
    Projection,
    Cast,

    // Literals.
    StringLiteral,
    NumericLiteral,

    // Values.
    UnitLikeValue,
    DeferredValue,
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

struct Parameter
{
    Variable const* variable;
    TermPtr expected_type;
    Parameter(Variable const* variable, TermPtr expected_type)
        : variable(variable), expected_type(expected_type)
    {}
    bool operator==(const Parameter& y) const
    {
        return variable == y.variable && expected_type == y.expected_type;
    }
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
        snl::hash_combine(h, x.expected_type);
        return h;
    }
};
template <>
struct hash<snl::term::BoundVariable>
{
    std::size_t operator()(const snl::term::BoundVariable& x) const noexcept
    {
        auto h = snl::hash_value(x.variable);
        snl::hash_combine(h, x.value);
        return h;
    }
};
}  // namespace std

namespace snl {
namespace term {

// Introduces a number of free variables for the body. Part of these variables cab be bound by
// supplying the abstraction to an application. Others can be bound here.
// The type must reflect the number of arguments this term expects.
// If the number of arguments is zero, this is a value term, no need to put into application.
struct Abstraction : Term
{
    STATIC_TAG(Abstraction);

    /*
    private:
        static TermPtr TypeFromParametersAndResult(Store& store,
                                                   const vector<Parameter>& parameters,
                                                   TermPtr result_type);
         */
public:
    // If one parameter's expected_type refers another parameter the other parameter must be
    // applied first.
    // Parameter's expected_type cannot refer to itself.
    // bound_variables' values cannot refer a parameter.
    // A bound variable cannot refer another bound variable which is listed
    // after it.
    vector<BoundVariable> bound_variables;
    vector<Parameter> parameters;
    TermPtr body;

    Abstraction(vector<BoundVariable>&& bound_variables,
                vector<Parameter>&& parameters,
                TermPtr body)
        : Term(Tag::Abstraction),
          bound_variables(move(bound_variables)),
          parameters(move(parameters)),
          body(body)
    {}
};

// Introduces universally quantified free variables for the term. These variables must be resolved
// compile time. Traditionally these are type variables but here we might have
// - parameters marked as `comptime` and used in types of subsequent parameters or local variables
// - parameters marked as `comptime` and not even used in type expressions.
// These parameters must be listed in the enclosing ForAll term which lists all the variables
// which must be resolved before we can monomorphise and compile the quantified term.
struct ForAll : Term
{
    STATIC_TAG(ForAll);
    unordered_set<Variable const*> variables;  // Universally quantified.
    TermPtr term;
    ForAll(unordered_set<Variable const*>&& variables, TermPtr term)
        : Term(Tag::ForAll), variables(move(variables)), term(term)
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

struct Projection : Term
{
    STATIC_TAG(Projection);

    TermPtr domain;
    string codomain;
    Projection(TermPtr domain, string&& codomain)
        : Term(Tag::Projection), domain(domain), codomain(move(codomain))
    {}
};

struct Cast : Term
{
    STATIC_TAG(Cast);

    TermPtr subject;
    TermPtr target_type;
    Cast(TermPtr subject, TermPtr target_type)
        : Term(Tag::Cast), subject(subject), target_type(target_type)
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

    string value;
    NumericLiteral(string&& value) : Term(Tag::StringLiteral), value(move(value)) {}
};

struct TypeTerm : Term
{
    explicit TypeTerm(Tag tag) : Term(tag) {}
};

// TODO this terms could be combined into a single BuiltInType.
struct TypeOfTypes : TypeTerm
{
    STATIC_TAG(TypeOfTypes);

    TypeOfTypes() : TypeTerm(Tag::TypeOfTypes) {}
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

    vector<TypeAndAvailability> parameter_types;
    TermPtr result_type;
    FunctionType(vector<TypeAndAvailability>&& parameter_types, TermPtr result_type)
        : TypeTerm(Tag::FunctionType),
          parameter_types(move(parameter_types)),
          result_type(result_type)
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
        snl::hash_combine(h, x.type);
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
    optional<int> FindMemberIndex(const string& tag) const
    {
        auto it = std::find_if(BE(members), [&tag](auto& tt) { return tt.tag == tag; });
        if (it == members.end()) {
            return nullopt;
        }
        return it - members.begin();
    }
    explicit ProductType(vector<TaggedType>&& members)
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
    } role;
    DeferredValue(TermPtr type, Availability role) : ValueTerm(Tag::DeferredValue, type), role(role)
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
    vector<TermPtr> values;
    ProductValue(TermPtr type, vector<TermPtr>&& values)
        : ValueTerm(Tag::ProductValue, type), values(move(values))
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
