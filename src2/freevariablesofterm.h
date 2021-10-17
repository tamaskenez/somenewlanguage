#pragma once

#include "term.h"

namespace snl {
struct FreeVariables
{
    enum class VariableUsage
    {
        FlowsIntoType,  // And maybe into value, too.
        FlowsIntoValue
    };
    std::unordered_map<term::Variable const*, VariableUsage> variables;
    FreeVariables CopyToTypeLevel() const;
    bool EraseVariable(term::Variable const* v);
    void InsertWithKeepingStronger(const FreeVariables& fv);

    void InsertWithFlowingToType(const vector<term::Variable const*> variables)
    {
        for (auto v : variables) {
            this->variables.insert(make_pair(v, VariableUsage::FlowsIntoType));
        }
    }

    bool DoesFlowIntoType(term::Variable const* v) const;
    bool operator==(const FreeVariables& y) const { return variables == y.variables; }
};

FreeVariables const* GetFreeVariables(Store& store, TermPtr term);

}  // namespace snl

namespace std {
template <>
struct hash<snl::FreeVariables>
{
    std::size_t operator()(const snl::FreeVariables& x) const noexcept
    {
        return snl::hash_range(BE(x.variables));
    }
};
}  // namespace std
