#pragma once

#include "term.h"

#include <optional>

namespace snl {

struct BoundVariables
{
    unordered_map<term::Variable const*, term::TermPtr> variables;
    optional<term::TermPtr> LookUp(term::Variable const* variable) const
    {
        auto it = variables.find(variable);
        return it == variables.end() ? nullopt : make_optional(it->second);
    }
    void Bind(term::Variable const* variable, term::TermPtr value)
    {
        auto itb = variables.insert(make_pair(variable, value));
        assert(itb.second);
        (void)itb;
    }
    void append(BoundVariables&& y)
    {
        if (variables.empty()) {
            variables = std::move(y.variables);
        } else {
            variables.insert(BE(y.variables));
        }
    }
};

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
    bool eval_values;  // Besides types, or types only.
    bool allow_unbound_variables;
    EvalContext(term::Store& store,
                const BoundVariablesWithParent& context,
                bool eval_values,
                bool allow_unbound_variables)
        : store(store),
          context(context),
          eval_values(eval_values),
          allow_unbound_variables(allow_unbound_variables)
    {}
    EvalContext MakeInner(const BoundVariablesWithParent& inner_context) const
    {
        return EvalContext{store, inner_context, eval_values, allow_unbound_variables};
    }
};

optional<term::TermPtr> EvaluateTerm(const EvalContext& ec, term::TermPtr term);

}  // namespace snl
