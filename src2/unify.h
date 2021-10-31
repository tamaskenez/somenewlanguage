#pragma once

#include "common.h"
#include "term.h"

namespace snl {
struct Store;

struct UnifyResult
{
    TermPtr resolved_pattern;
    unordered_map<term::Variable const, TermPtr> new_bound_variables;
};

optional<UnifyResult> Unify(Store& store,
                            const Context& context,
                            TermPtr pattern,
                            TermPtr concrete,
                            const unordered_set<term::Variable const*>& variables_to_unify);

struct UETATResult
{
    vector<BoundVariable> bound_variables;
    TermPtr cast_arg;
};

// Both expected_type and arg_type must be evaluated, only variables allowed to occur are the forall
// variables.
optional<UETATResult> UnifyExpectedTypeToArgType(
    TermPtr expected_type,
    TermPtr arg_type,
    const unordered_set<term::Variable const*>& forall_variables);

}  // namespace snl
