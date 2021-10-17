#pragma once

#include "context.h"
#include "term.h"

#include <optional>

namespace snl {

/*
struct InitialPassContext
{
    Store& store;
    const Context& context;
    InitialPassContext(Store& store, const Context& context) : store(store), context(context) {}
    InitialPassContext DuplicateWithDifferentContext(const Context& inner_context) const
    {
        return InitialPassContext{store, inner_context};
    }
};

struct InitialPassError
{};

optional<InitialPassError> InitialPass(const InitialPassContext& ipc, TermPtr term);

struct EvalContext
{
    Store& store;
    const Context& context;
    bool eval_values;  // Eval values and types (true), or types only (false).
    EvalContext(Store& store, const Context& context, bool eval_values)
        : store(store), context(context), eval_values(eval_values)
    {}
    EvalContext DuplicateWithDifferentContext(const Context& inner_context) const
    {
        return EvalContext{store, inner_context, eval_values};
    }
    EvalContext DuplicateWithEvalValues() const { return EvalContext{store, context, true}; }
};

struct EvaluateTermError
{
    enum class Tag
    {
        UnboundVariable
    } tag;
};

using EvaluateTermResult = either<EvaluateTermError, TermPtr>;

optional<TermPtr> EvaluateTerm(const EvalContext& ec, TermPtr term);
*/

optional<TermPtr> CompileTerm(Store& store, const Context& context, TermPtr term);

optional<TermPtr> EvaluateTerm(Store& store, const Context& context, TermPtr term);

optional<vector<TermPtr>> InferTypeOfTerms(Store& store,
                                           const Context& context,
                                           const vector<TermPtr>& terms);

struct InferCalleeTypesResult
{
    vector<TermPtr> bound_parameter_types;
    unordered_set<term::Variable const*> remaining_forall_variables;
    vector<TermPtr> remaining_parameter_types;
    TermPtr result_type;
};

optional<InferCalleeTypesResult> InferCalleeTypes(Store& store,
                                                  const Context& context,
                                                  TermPtr callee_term,
                                                  const vector<TermPtr>& argument_types);
optional<TermPtr> InferTypeOfTerm(Store& store, const Context& context, TermPtr term);
}  // namespace snl
