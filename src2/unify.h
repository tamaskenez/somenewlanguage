#pragma once

#include "common.h"
#include "context.h"
#include "term_forward.h"

namespace snl {
struct Store;

struct UnifyResult
{
    TermPtr resolved_pattern;
    BoundVariables new_bound_variables;
};

optional<UnifyResult> Unify(Store& store,
                            const Context& context,
                            TermPtr pattern,
                            TermPtr concrete,
                            const unordered_set<term::Variable const*>& variables_to_unify);
}  // namespace snl
