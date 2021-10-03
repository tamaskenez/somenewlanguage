#pragma once

#include "common.h"

#include "term_forward.h"

namespace snl {
struct BoundVariables
{
    unordered_map<term::Variable const*, term::TermPtr> variables;
    optional<term::TermPtr> LookUp(term::Variable const* variable) const;
    void Bind(term::Variable const* variable, term::TermPtr value);
    void Append(BoundVariables&& y);
};
}  // namespace snl
