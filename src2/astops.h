#pragma once

#include "term.h"

#include <optional>

namespace snl {

struct BoundVariablesWithParent : BoundVariables
{
    BoundVariablesWithParent const* const parent = nullptr;

    explicit BoundVariablesWithParent(BoundVariablesWithParent const* parent) : parent(parent) {}

    optional<term::TermPtr> LookUp(term::Variable const* variable) const
    {
        if (auto term = BoundVariables::LookUp(variable)) {
            return term;
        }
        return parent ? parent->LookUp(variable) : nullopt;
    }
    void Bind(term::Variable const* variable, term::TermPtr value)
    {
        assert(!LookUp(variable));
        BoundVariables::Bind(variable, value);
    }
};

struct EvalContext
{
    term::Store& store;
    const BoundVariablesWithParent& context;
    bool eval_values;              // Eval values and types (true), or types only (false).
    bool allow_unbound_variables;  // Doesn't apply for all terms.
    EvalContext(term::Store& store,
                const BoundVariablesWithParent& context,
                bool eval_values,
                bool allow_unbound_variables)
        : store(store),
          context(context),
          eval_values(eval_values),
          allow_unbound_variables(allow_unbound_variables)
    {}
    EvalContext DuplicateWithDifferentContext(const BoundVariablesWithParent& inner_context) const
    {
        return EvalContext{store, inner_context, eval_values, allow_unbound_variables};
    }
    EvalContext DuplicateAndDontAllowUnboundVariables() const
    {
        return EvalContext{store, context, eval_values, false};
    }
    EvalContext DuplicateWithEvalValues() const
    {
        return EvalContext{store, context, true, allow_unbound_variables};
    }
};

struct EvaluateTermError
{
    enum class Tag
    {
        UnboundVariable
    } tag;
};

using EvaluateTermResult = either<EvaluateTermError, term::TermPtr>;

optional<term::TermPtr> EvaluateTerm(const EvalContext& ec, term::TermPtr term);

}  // namespace snl
