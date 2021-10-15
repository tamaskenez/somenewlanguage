#pragma once

#include "common.h"

#include "term_forward.h"

namespace snl {

struct BoundVariables
{
    unordered_map<term::Variable const*, TermPtr> variables;

    BoundVariables() = default;
    BoundVariables(const BoundVariables&) = delete;
    BoundVariables(BoundVariables&&) = default;

    optional<TermPtr> LookUp(term::Variable const* variable) const;
    void Bind(term::Variable const* variable, TermPtr value);
    void Rebind(term::Variable const* variable, TermPtr value);
    void Append(BoundVariables&& y);
};

struct Context : BoundVariables
{
    Context const* const parent = nullptr;

    Context(const Context&) = delete;
    explicit Context(Context const* parent) : parent(parent) {}

    optional<TermPtr> LookUp(term::Variable const* variable) const;
    void Bind(term::Variable const* variable, TermPtr value);
    void Rebind(term::Variable const* variable, TermPtr value);
};
}  // namespace snl
