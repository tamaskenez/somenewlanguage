#pragma once

#include "common.h"
#include "freevariablesofterm.h"
#include "term.h"

namespace snl {
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
    bool operator==(const TermWithBoundFreeVariables& y) const
    {
        return term == y.term && bound_variables == y.bound_variables;
    }
};

}  // namespace snl

namespace std {
template <>
struct hash<snl::TermWithBoundFreeVariables>
{
    std::size_t operator()(const snl::TermWithBoundFreeVariables& x) const noexcept
    {
        auto h = snl::hash_value(x.term);
        snl::hash_combine(h, x.bound_variables);
        return h;
    }
};
}  // namespace std

namespace snl {
struct Store
{
    Store();
    ~Store();

    TermPtr MakeCanonical(Term&& term);
    FreeVariables const* MakeCanonical(FreeVariables&& fv);
    bool IsCanonical(TermPtr x) const;
    term::Variable const* MakeNewVariable(string&& name = string());
    bool DoesVariableFlowIntoType(term::Variable const* v) const;
    optional<TermPtr> GetOrInsertTypeOfTermInContext(
        TermWithBoundFreeVariables&& term_with_bound_free_variables,
        std::function<optional<TermPtr>()> make_type_fn);

    unordered_set<TermPtr, TermHash, TermEqual> canonical_terms;

    TermPtr const type_of_types;
    TermPtr const bottom_type;
    TermPtr const unit_type;
    TermPtr const top_type;
    TermPtr const string_literal_type;
    TermPtr const numeric_literal_type;
    TermPtr const comptime_type_value;
    TermPtr const comptime_value_comptime_type;

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
