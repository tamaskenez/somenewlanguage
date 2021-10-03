#include "bound_variables.h"

namespace snl {

optional<term::TermPtr> BoundVariables::LookUp(term::Variable const* variable) const
{
    auto it = variables.find(variable);
    return it == variables.end() ? nullopt : make_optional(it->second);
}
void BoundVariables::Bind(term::Variable const* variable, term::TermPtr value)
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
}  // namespace snl
