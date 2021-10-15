#pragma once

#include "common.h"

#include "context.h"
#include "term_forward.h"

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

    string name;  // For diagnostics.
    explicit Variable(string&& name) : Term(Tag::Variable), name(move(name)) {}
};

struct Parameter
{
    Variable const* variable;
    TermPtr expected_type;
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
        snl::hash_combine(h, snl::hash_value(x.expected_type));
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
// compile time. Usually they are type variables flowing into type level but there's no such
// restriction: They can be non-type variables or they can flow to value level.
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
    optional<TaggedType const*> FindMember(const string& tag) const
    {
        auto it = std::find_if(BE(members), [&tag](auto& tt) { return tt.tag == tag; });
        if (it == members.end()) {
            return nullopt;
        }
        return &*it;
    }
    explicit ProductType(vector<TaggedType>&& members)
        : TypeTerm(Tag::ProductType), members(move(members))
    {}
};

// An unspecified value which will be resolved later, comptime or runtime.
struct DeferredValue : ValueTerm
{
    STATIC_TAG(DeferredValue);
    enum class Role
    {
        Runtime,
        UniversallyQualifiedComptimeMarkedForUnification
    } role;
    DeferredValue(TermPtr type, Role role) : ValueTerm(Tag::DeferredValue, type), role(role) {}
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

// Store.

struct FreeVariables
{
    enum class VariableUsage
    {
        FlowsIntoType,  // And maybe into value, too.
        FlowsIntoValue
    };
    std::unordered_map<term::Variable const*, VariableUsage> variables;
    FreeVariables CopyToTypeLevel() const
    {
        FreeVariables result;
        for (auto [k, v] : variables) {
            result.variables.insert(make_pair(k, VariableUsage::FlowsIntoType));
        }
        return result;
    }
    bool EraseVariable(term::Variable const* v) { return variables.erase(v) == 1; }
    void InsertWithKeepingStronger(const FreeVariables& fv)
    {
        for (auto [k, v] : fv.variables) {
            switch (v) {
                case VariableUsage::FlowsIntoType:
                    variables[k] = VariableUsage::FlowsIntoType;
                    break;
                case VariableUsage::FlowsIntoValue:
                    // Insert only if it's not there.
                    variables.insert(make_pair(k, VariableUsage::FlowsIntoValue));
                    break;
            }
        }
    }
    template <class It>
    void InsertWithFlowingToType(It b, It e)
    {
        for (auto it = b; it != e; ++it) {
            variables.insert(make_pair(*it, VariableUsage::FlowsIntoType));
        }
    }
    bool DoesFlowIntoType(term::Variable const* v) const
    {
        auto it = variables.find(v);
        return it != variables.end() && it->second == VariableUsage::FlowsIntoType;
    }
};

struct AboutVariable
{
    // Variable flowing into type: used in a term which describes another variable's type.
    // This variable can still be anything, like an integer.
    bool flows_into_type = false;
};

struct TermWithBoundFreeVariables
{
    TermPtr term;
    BoundVariables bound_variables;
    TermWithBoundFreeVariables(TermPtr term, BoundVariables&& bound_variables)
        : term(term), bound_variables(move(bound_variables))
    {}
};

struct Store
{
    Store();
    ~Store();

    TermPtr MakeCanonical(Term&& term);
    FreeVariables const* MakeCanonical(FreeVariables&& fv);
    bool IsCanonical(TermPtr x) const;
    term::Variable const* MakeNewVariable(string&& name = string());
    bool DoesVariableFlowIntoType(term::Variable const* v) const
    {
        auto it = about_variables.find(v);
        return it != about_variables.end() && it->second.flows_into_type;
    }

    unordered_set<TermPtr, TermHash, TermEqual> canonical_terms;

    TermPtr const type_of_types;
    TermPtr const bottom_type;
    TermPtr const unit_type;
    TermPtr const top_type;
    TermPtr const string_literal_type;
    TermPtr const numeric_literal_type;

    unordered_map<TermPtr, TermPtr> memoized_comptime_applications;
    unordered_set<FreeVariables> canonicalized_free_variables;
    unordered_map<TermPtr, FreeVariables const*> free_variables_of_terms;
    unordered_map<term::Variable const*, AboutVariable> about_variables;
    unordered_map<TermWithBoundFreeVariables, TermPtr> types_of_terms_in_context;

    static string const s_ignored_name;

private:
    TermPtr MoveToHeap(Term&& t);
    int next_generated_variable_id = 1;
};

}  // namespace snl
