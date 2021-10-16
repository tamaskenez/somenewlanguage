#pragma once

#include "common.h"
#include "term.h"

namespace snl {  // Store.

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
    bool operator==(const FreeVariables& y) const { return variables == y.variables; }
};
}  // namespace snl

namespace std {
template <>
struct hash<snl::FreeVariables>
{
    std::size_t operator()(const snl::FreeVariables& x) const noexcept
    {
        // return snl::hash_range(BE(x.variables));
        return 0;
    }
};
}  // namespace std

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
        // return snl::hash_range(BE(x.variables));
        return 0;
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
