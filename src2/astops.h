#pragma once

#include "context.h"
#include "term.h"

#include <optional>

namespace snl {

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

}  // namespace snl
