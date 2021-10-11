#include "bound_variables.h"

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
void BoundVariables::Append(BoundVariables&& y)
{
    if (variables.empty()) {
        variables = std::move(y.variables);
    } else {
        variables.insert(BE(y.variables));
    }
}
optional<TermPtr> BoundVariablesWithParent::LookUp(term::Variable const* variable) const
{
    if (auto term = BoundVariables::LookUp(variable)) {
        return term;
    }
    return parent ? parent->LookUp(variable) : nullopt;
}
void BoundVariablesWithParent::Bind(term::Variable const* variable, TermPtr value)
{
    assert(!LookUp(variable));
    BoundVariables::Bind(variable, value);
}
}  // namespace snl
