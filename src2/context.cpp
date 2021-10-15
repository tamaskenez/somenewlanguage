#include "context.h"

namespace snl {

optional<TermPtr> BoundVariables::LookUp(term::Variable const* variable) const
{
    auto it = variables.find(variable);
    return it == variables.end() ? nullopt : make_optional(it->second);
}
void BoundVariables::Bind(term::Variable const* variable, TermPtr value)
{
    auto itb = variables.insert(make_pair(variable, value));
    assert(itb.second);
    (void)itb;
}
void BoundVariables::Rebind(term::Variable const* variable, TermPtr value)
{
    auto it = variables.find(variable);
    ASSERT_ELSE(it != variables.end(), {
        variables.insert(make_pair(variable, value));
        return;
    })
    it->second = value;
}
void BoundVariables::Append(BoundVariables&& y)
{
    if (variables.empty()) {
        variables = std::move(y.variables);
    } else {
        variables.insert(BE(y.variables));
    }
}
optional<TermPtr> Context::LookUp(term::Variable const* variable) const
{
    if (auto term = BoundVariables::LookUp(variable)) {
        return term;
    }
    return parent ? parent->LookUp(variable) : nullopt;
}
void Context::Bind(term::Variable const* variable, TermPtr value)
{
    assert(!LookUp(variable));
    BoundVariables::Bind(variable, value);
}
void Context::Rebind(term::Variable const* variable, TermPtr value)
{
    assert(!parent || !parent->LookUp(variable));
    BoundVariables::Rebind(variable, value);
}
}  // namespace snl
