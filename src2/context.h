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

    bool operator==(const BoundVariables& y) const { return variables == y.variables; }
};

}  // namespace snl

namespace std {
template <>
struct hash<snl::BoundVariables>
{
    std::size_t operator()(const snl::BoundVariables& x) const noexcept
    {
        return snl::hash_range(BE(x.variables));
    }
};
}  // namespace std

namespace snl {

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
