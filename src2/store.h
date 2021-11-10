#pragma once

#include "builtin_function.h"
#include "common.h"
#include "freevariablesofterm.h"
#include "term.h"

namespace snl {
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
    term::Variable const* MakeNewVariable(bool comptime, string&& name = string());
    optional<TermPtr> GetOrInsertTypeOfTermInContext(
        TermWithBoundFreeVariables&& term_with_bound_free_variables,
        std::function<optional<TermPtr>()> make_type_fn);
    int AddInnerFunctionDefinition(InnerFunctionDefinition&& ifd)
    {
        int id = next_inner_function_id++;
        bool added = inner_function_map.insert(make_pair(id, move(ifd))).second;
        assert(added);
        return id;
    }

    unordered_set<TermPtr, TermHash, TermEqual> canonical_terms;

    TermPtr const type_of_types;
    TermPtr const unit_type;
    TermPtr const string_literal_type;
    TermPtr const numeric_literal_type;
    TermPtr const comptime_type_value;
    TermPtr const comptime_value_comptime_type;

    InnerFunctionMap inner_function_map;
    int next_inner_function_id = 0;
    BuiltinFunctionMap builtin_function_map;

    unordered_set<FreeVariables> canonicalized_free_variables;
    unordered_map<TermPtr, FreeVariables const*> free_variables_of_terms;
    unordered_map<TermWithBoundFreeVariables, TermPtr> types_of_terms_in_context;

    static string const s_ignored_name;

private:
    TermPtr MoveToHeap(Term&& t);
    int next_generated_variable_id = 1;
};
}  // namespace snl
