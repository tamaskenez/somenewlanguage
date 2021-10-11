#pragma once

#include "common.h"

#include "term_forward.h"

namespace snl {

struct BoundVariables
{
    unordered_map<term::Variable const*, TermPtr> variables;

    BoundVariables() = default;
    BoundVariables(const BoundVariables&) = delete;

    optional<TermPtr> LookUp(term::Variable const* variable) const;
    void Bind(term::Variable const* variable, TermPtr value);
    void Append(BoundVariables&& y);
};

struct BoundVariablesWithParent : BoundVariables
{
    BoundVariablesWithParent const* const parent = nullptr;

    BoundVariablesWithParent(const BoundVariablesWithParent&) = delete;
    explicit BoundVariablesWithParent(BoundVariablesWithParent const* parent) : parent(parent) {}

    optional<TermPtr> LookUp(term::Variable const* variable) const;
    void Bind(term::Variable const* variable, TermPtr value);
};
}  // namespace snl
