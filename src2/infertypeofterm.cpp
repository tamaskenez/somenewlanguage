#include "astops.h"

#include "freevariablesofterm.h"
#include "store.h"
#include "unify.h"

namespace snl {

optional<TermPtr> InferTypeOfTermCore(Store& store, const Context& context, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction: {
            auto* tc = term_cast<term::Abstraction>(term);
            Context inner_context(&context);
            // By calling GetFreeVariables we make sure all bound variables are registered,
            // their comptime-ness is available.
            GetFreeVariables(store, term);
            for (auto bv : tc->bound_variables) {
                auto value_type = InferTypeOfTerm(store, inner_context, bv.value);
                if (!value_type) {
                    return nullopt;
                }
                inner_context.Bind(bv.variable,
                                   store.MakeCanonical(term::DeferredValue(
                                       *value_type, store.DoesVariableFlowIntoType(bv.variable)
                                                        ? term::DeferredValue::Role::Comptime
                                                        : term::DeferredValue::Role::Runtime)));
            }
            for (auto p : tc->parameters) {
                auto parameter_type = EvaluateTerm(store, inner_context, p.expected_type);
                if (!parameter_type) {
                    return nullopt;
                }
                inner_context.Bind(p.variable,
                                   store.MakeCanonical(term::DeferredValue(
                                       *parameter_type, store.DoesVariableFlowIntoType(p.variable)
                                                            ? term::DeferredValue::Role::Comptime
                                                            : term::DeferredValue::Role::Runtime)));
            }
            return InferTypeOfTerm(store, inner_context, tc->body);
        }
        case Tag::Application: {
            auto* tc = term_cast<term::Application>(term);
            auto m_argument_types = InferTypeOfTerms(store, context, tc->arguments);
            if (!m_argument_types) {
                return nullopt;
            }
            auto& argument_types = *m_argument_types;
            auto callee_types = InferCalleeTypes(store, context, tc->function, argument_types);
            if (!callee_types) {
                return nullopt;
            }
            if (callee_types->remaining_parameter_types.empty()) {
                assert(callee_types->remaining_forall_variables.empty());
                return callee_types->result_type;
            }
            auto new_function_type = store.MakeCanonical(term::FunctionType(
                move(callee_types->remaining_parameter_types), callee_types->result_type));
            return callee_types->remaining_forall_variables.empty()
                       ? new_function_type
                       : store.MakeCanonical(term::ForAll(
                             move(callee_types->remaining_forall_variables), new_function_type));
        }
        case Tag::ForAll: {
            auto* tc = term_cast<term::ForAll>(term);
            ASSERT_ELSE(!tc->variables.empty(), return InferTypeOfTerm(store, context, tc->term););
            Context inner_context(&context);
            for (auto v : tc->variables) {
                inner_context.Bind(v, store.comptime_value_comptime_type);
            }
            auto inner_type = InferTypeOfTerm(store, inner_context, tc->term);
            if (!inner_type) {
                return nullopt;
            }
            return store.MakeCanonical(term::ForAll(make_copy(tc->variables), *inner_type));
        }
        case Tag::Cast: {
            auto* cast = term_cast<term::Cast>(term);
            return EvaluateTerm(store, context, cast->target_type);
        }
        case Tag::Variable:
        case Tag::Projection:
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::UnitLikeValue:
        case Tag::DeferredValue:
        case Tag::ProductValue:
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
        case Tag::FunctionType:
        case Tag::ProductType:
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
            assert(false);
            return nullopt;
    }
}

optional<vector<TermPtr>> InferTypeOfTerms(Store& store,
                                           const Context& context,
                                           const vector<TermPtr>& terms)
{
    vector<TermPtr> result;
    for (auto term : terms) {
        auto type = InferTypeOfTerm(store, context, term);
        if (!type) {
            return nullopt;
        }
        result.push_back(*type);
    }
    return result;
}

// TODO InferCalleeTypes needs to receive the actual arguments since a comptime parameter's value
// can be used in the next parameter's type.
optional<InferCalleeTypesResult> InferCalleeTypes(Store& store,
                                                  const Context& context,
                                                  TermPtr callee_term,
                                                  const vector<TermPtr>& argument_types)
{
    auto m_callee_type = InferTypeOfTerm(store, context, callee_term);
    if (!m_callee_type) {
        return nullopt;
    }
    term::FunctionType const* function_type = nullptr;
    term::ForAll const* for_all = nullptr;

    using Tag = term::Tag;
    if ((*m_callee_type)->tag == Tag::FunctionType) {
        function_type = term_cast<term::FunctionType>(*m_callee_type);
    } else if ((*m_callee_type)->tag == Tag::ForAll) {
        for_all = term_cast<term::ForAll>(*m_callee_type);
        if (for_all->term->tag == Tag::FunctionType) {
            function_type = term_cast<term::FunctionType>(for_all->term);
        }
    }
    if (!function_type) {
        return nullopt;
    }

    // Now we have the function_type and an optional for_all universal qualifier.
    int n_pars = function_type->parameter_types.size();
    int n_args = argument_types.size();
    if (n_args > n_pars) {
        return nullopt;
    }

    // At this point n_args <= n_pars.
    unordered_set<term::Variable const*> forall_variables;
    Context inner_context(&context);

    if (for_all) {
        forall_variables = for_all->variables;
        for (auto v : forall_variables) {
            inner_context.Bind(v, store.comptime_value_comptime_type);
        }
    }

    vector<TermPtr> parameter_types;

    for (int i = 0; i < std::min(n_args, n_pars); ++i) {
        auto par_type = function_type->parameter_types[i];  // Might contain variables from for_all
        // Unify type of arg_type to par_type and put the newly bound variables into current
        // context. They must be a variable from the for_all context.
        auto arg_type = argument_types[i];
        auto ur = Unify(store, inner_context, par_type, arg_type, forall_variables);
        // Add resolved variables to the inner_context, remove from universal_variables
        if (!ur) {
            return nullopt;
        }
        for (auto [var, val] : ur->new_bound_variables.variables) {
            ASSERT_ELSE(forall_variables.count(var) > 0, return nullopt;);
            forall_variables.erase(var);
            inner_context.Rebind(var, val);
        }
        parameter_types.push_back(ur->resolved_pattern);
    }

    vector<TermPtr> remaining_parameter_types;
    for (int i = n_args; i < n_pars; ++i) {
        auto type = EvaluateTerm(store, inner_context, function_type->parameter_types[i]);
        if (!type) {
            return nullopt;
        }
        remaining_parameter_types.push_back(*type);
    }
    auto result_type = EvaluateTerm(store, inner_context, function_type->result_type);
    if (!result_type) {
        return nullopt;
    }
    return InferCalleeTypesResult{move(parameter_types), move(forall_variables),
                                  move(remaining_parameter_types), *result_type};
}

optional<TermPtr> InferTypeOfTerm(Store& store, const Context& context, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction:
        case Tag::Application:
        case Tag::ForAll:
        case Tag::Cast:
            break;

        case Tag::Variable: {
            auto value = context.LookUp(term_cast<term::Variable>(term));
            ASSERT_ELSE(value, return nullopt;);
            return InferTypeOfTerm(store, context, *value);
        }
        case Tag::Projection: {
            auto* projection = term_cast<term::Projection>(term);

            VAL_FROM_OPT_ELSE_RETURN(domain_type,
                                     InferTypeOfTerm(store, context, projection->domain), nullopt);

            if (domain_type->tag != Tag::ProductType) {
                return nullopt;
            }
            auto product_type = term_cast<term::ProductType>(domain_type);

            VAL_FROM_OPT_ELSE_RETURN(member_index,
                                     product_type->FindMemberIndex(projection->codomain), nullopt);
            auto& tt = product_type->members[member_index];
            return tt.type;
        }

        case Tag::UnitLikeValue:
            return EvaluateTerm(store, context, term_cast<term::UnitLikeValue>(term)->type);
        case Tag::DeferredValue:
            return EvaluateTerm(store, context, term_cast<term::DeferredValue>(term)->type);
        case Tag::ProductValue:
            return EvaluateTerm(store, context, term_cast<term::ProductValue>(term)->type);

        case Tag::StringLiteral:
            return store.string_literal_type;
        case Tag::NumericLiteral:
            return store.numeric_literal_type;
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
        case Tag::FunctionType:
        case Tag::ProductType:
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
            return store.type_of_types;
    }
    auto fv = GetFreeVariables(store, term);
    // All must be bound.
    BoundVariables term_context;
    for (auto [var, role] : fv->variables) {
        auto value = context.LookUp(var);
        ASSERT_ELSE(value, return nullopt;);
        term_context.Bind(var, *value);
    }
    return store.GetOrInsertTypeOfTermInContext(
        TermWithBoundFreeVariables(term, move(term_context)),
        [&store, &context, term]() -> optional<TermPtr> {
            return InferTypeOfTermCore(store, context, term);
        });
}

// TODO introduce some attribute for abstractions:
// - user functions with 3 inlining levels: never, auto, always
// - non-user functions
// the abstraction boundary of non-user functions and always/auto functions can be optimized
// away (merged with an application or an abstraction)

// TODO test whether we can produce for-all immediately inside a for-all and how it compiles and
// what to do
// TODO test what happens with for-all not immediately inside a for-all
// TODO add some device to mark operands in a function type comptime. Or maybe we don't need it?

// TODO Instead of flow-into-type I think we always need comptime-ness.

}  // namespace snl
