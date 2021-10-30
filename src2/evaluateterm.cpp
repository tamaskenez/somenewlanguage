#include "astops.h"

#include "evaluateorcompileterm.h"
#include "store.h"

namespace snl {
struct UETEVResult
{};
optional<UETEVResult> UnifyExpectedTypeToEvaluatedValue(
    Store& store,
    Context& context,
    TermPtr expected_type,
    TermPtr evaluated_arg,
    const unordered_set<term::Variable const*>& remaining_forall_variables)
{
    return nullopt;
}

optional<TermPtr> EvaluateTerm(Store& store, const Context& context, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction: {
            auto* abstraction = term_cast<term::Abstraction>(term);
            // An abstraction is either a scope with yet unevaluated bound values and no parameters
            // in this it's simply a lazy expression, we need to evaluate it.
            // or it starts as an actual abstraction with parameters and without bound values
            // and we might gradually convert parameters plus incoming arguments into evaluated
            // bound values.
            if (abstraction->parameters.empty()) {
                Context inner_context(&context);
                for (auto& bv : abstraction->bound_variables) {
                    VAL_FROM_OPT_ELSE_RETURN(evaluated_value,
                                             EvaluateTerm(store, inner_context, bv.value), nullopt);
                    inner_context.Bind(bv.variable, evaluated_value);
                }
                return EvaluateTerm(store, inner_context, abstraction->body);
            }
            // This is an abstraction with unbound parameters, it's evaluation is itself
            return term;
        }
        case Tag::Application: {
            auto* application = term_cast<term::Application>(term);
            ASSERT_ELSE(!application->arguments.empty(), return nullopt;);

            // Investigate `application->function`
            VAL_FROM_OPT_ELSE_RETURN(evaluated_function,
                                     EvaluateTerm(store, context, application->function), nullopt);

            // Expect that the evaluated_function is "for_all abstraction" or "abstraction"
            using Tag = term::Tag;
            term::Abstraction const* abstraction = nullptr;
            term::ForAll const* for_all = nullptr;
            if (evaluated_function->tag == Tag::Abstraction) {
                abstraction = term_cast<term::Abstraction>(evaluated_function);
            } else if (evaluated_function->tag == Tag::ForAll) {
                for_all = term_cast<term::ForAll>(evaluated_function);
                if (for_all->term->tag == Tag::Abstraction) {
                    abstraction = term_cast<term::Abstraction>(for_all->term);
                }
            }
            if (!abstraction) {
                return nullopt;
            }

            // n_args n_pars
            // 0      some   evaluate function and try again
            // some   more   evaluate some args/pars and try again with rest
            // more   some   evaluate some args/pars and return lambda

            int n_pars = abstraction->parameters.size();
            if (n_pars == 0) {  // Parameterless abstraction: evaluate and retry
                ASSERT_ELSE(!for_all, return nullopt;);
                return EvaluateTerm(
                    store, context,
                    new term::Application(evaluated_function, make_copy(application->arguments)));
            }

            // Add the bound variables to an inner context. These are expected to be evaluated
            // because only parameterless abstractions have unevaluated bound variables.
            // Abstractions with parameters started as normal functions without any bound variable.
            // All the bound variables present here are coming from evaluated arguments.

            Context inner_context(&context);
            for (auto bv : abstraction->bound_variables) {
                inner_context.Bind(bv.variable, bv.value);
            }

            // Evaluate and bind arguments to parameters.

            // inner_context, remaining_forall_variables and new_bound_variables will be updated in
            // sync.
            unordered_set<term::Variable const*> remaining_forall_variables;
            vector<BoundVariable> new_bound_variables;

            if (for_all) {
                ASSERT_ELSE(!for_all->variables.empty(), return nullopt;);
                remaining_forall_variables = for_all->variables;
                // Remaining forall variables will be bound as deferred comptime values because the
                // parameter's expected_types might contain yet-to-be-unified forall variables,
                // still, they need to be evaluated first as far as it gets.
                for (auto v : for_all->variables) {
                    inner_context.Bind(v, store.comptime_value_comptime_type);
                }
            }

            int n_args = application->arguments.size();
            int n_bound_pars = std::min(n_args, n_pars);
            for (int i = 0; i < n_bound_pars; ++i) {
                auto arg = application->arguments[i];
                VAL_FROM_OPT_ELSE_RETURN(evaluated_arg, EvaluateTerm(store, context, arg), nullopt);
                auto par = abstraction->parameters[i];
                VAL_FROM_OPT_ELSE_RETURN(evaluated_expected_type,
                                         EvaluateTerm(store, inner_context, par.expected_type),
                                         nullopt);

                // A special case is when the expected type is a yet-unbound forall variable.
                term::Variable const* expected_type_as_single_unbound_forall_variable = nullptr;
                if (evaluated_expected_type->tag == Tag::Variable) {
                    auto* variable = term_cast<term::Variable>(evaluated_expected_type);
                    if (remaining_forall_variables.count(variable) > 0) {
                        expected_type_as_single_unbound_forall_variable = variable;
                    }
                }

                if (evaluated_arg->tag == Tag::ForAll) {
                    // For now, forall-args can bind only to forall variables.
                    if (!expected_type_as_single_unbound_forall_variable) {
                        return nullopt;
                    }
                    auto* arg_for_all = term_cast<term::ForAll>(evaluated_arg);
                    if (arg_for_all->term->tag == Tag::Abstraction) {
                        auto unresolved_abstraction_type =
                            new term::SimpleTypeTerm(term::SimpleType::UnresolvedAbstraction);
                        inner_context.Rebind(expected_type_as_single_unbound_forall_variable,
                                             unresolved_abstraction_type);
                        new_bound_variables.push_back(
                            BoundVariable{expected_type_as_single_unbound_forall_variable,
                                          unresolved_abstraction_type});
                        ASSERT_ELSE(remaining_forall_variables.erase(
                                        expected_type_as_single_unbound_forall_variable),
                                    return nullopt;);
                        inner_context.Bind(par.variable, evaluated_arg);
                        new_bound_variables.push_back(BoundVariable{par.variable, evaluated_arg});
                    } else {
                        return nullopt;  // For now, only forall abstractions are supported.
                    }
                } else {
                    // Arg is a concrete, non-forall value.
                    MOVE_FROM_OPT_ELSE_RETURN(ur,
                                              UnifyExpectedTypeToEvaluatedValue(
                                                  store, inner_context, par.expected_type,
                                                  evaluated_arg, remaining_forall_variables),
                                              nullopt);
                    for (auto bv : ur.bound_variables) {
                        inner_context.Rebind(bv.variable, bv.value);
                        new_bound_variables.push_back(bv);
                        remaining_forall_variables.erase(bv.variable);
                    }
                    // The type in the cast is expected to be evaluated. `EvaluateTerm` will attempt
                    // to evaluate the type but we don't supply inner_context here, only context so
                    // if the type would not be evaluated it might contain forall variables not
                    // bound in the (outer) context.
                    VAL_FROM_OPT_ELSE_RETURN(evaluated_cast_arg,
                                             EvaluateTerm(store, context, ur.cast_arg), nullopt);
                    inner_context.Bind(par.variable, evaluated_cast_arg);
                    new_bound_variables.push_back(BoundVariable{par.variable, evaluated_cast_arg});
                }
            }

            // Now 3 possibilities
            // n_args < n_pars: return an abstraction with the new bound variables
            // n_args == n_pars: evaluate the body
            // n_args > n_pars: construct a new application with the remaining parameters and return
            // the evaluation of that.
            if (n_args < n_pars) {
                auto* new_abstraction = new term::Abstraction(
                    move(new_bound_variables),
                    vector<Parameter>(abstraction->parameters.begin() + n_bound_pars,
                                      abstraction->parameters.end()),
                    abstraction->body);
                if (remaining_forall_variables.empty()) {
                    return new_abstraction;
                }
                return new term::ForAll(move(remaining_forall_variables), new_abstraction);
            }

            // We consumed all the parameters, there can't be any unbound forall variable.
            ASSERT_ELSE(remaining_forall_variables.empty(), return nullopt;);

            VAL_FROM_OPT_ELSE_RETURN(
                evaluated_body, EvaluateTerm(store, inner_context, abstraction->body), nullopt);

            if (n_args == n_pars) {
                return evaluated_body;
            }

            assert(n_args > n_pars);

            return EvaluateTerm(
                store, context,
                new term::Application(evaluated_body,
                                      vector<TermPtr>(application->arguments.begin() + n_bound_pars,
                                                      application->arguments.end())));
        }
        case Tag::ForAll: {
            // ForAll probably needs to be changed soon.
            // - Abstraction gets a builtin forall clause.
            // - Abstraction always have at least one parameter
            // - Nullary Abstraction will be Lets (Let-Ins)
            // - There would no ForAll clause
            // - Use cases like Maybe.None will be handled with DontCareType
            // - Parametric polymorphic types are not instantiated as forall types, instead we only
            // go until
            //   the function that creates it (which would be a forall-abstraction). The type is
            //   only instantiated when we have all the parameters.
            auto* tc = term_cast<term::ForAll>(term);
            ASSERT_ELSE(tc->term->tag == Tag::Abstraction, return nullopt;);
            // If it's abstraction, we don't need to evaluate.
            return term;
        }
        case Tag::Cast: {
            auto* cast = term_cast<term::Cast>(term);
            VAL_FROM_OPT_ELSE_RETURN(new_subject, EvaluateTerm(store, context, cast->subject),
                                     nullopt);
            VAL_FROM_OPT_ELSE_RETURN(new_target_type,
                                     EvaluateTerm(store, context, cast->target_type), nullopt);
            VAL_FROM_OPT_ELSE_RETURN(subject_type, InferTypeOfTerm(store, context, new_subject),
                                     nullopt);
            if (new_target_type == subject_type) {
                return new_subject;
            }
            ASSERT_ELSE(false, return nullopt;);
            // TODO have no idea how to cast. We should call the built-in `cast` function.
        }
        case Tag::Projection: {
            auto* projection = term_cast<term::Projection>(term);
            VAL_FROM_OPT_ELSE_RETURN(new_domain, EvaluateTerm(store, context, projection->domain),
                                     nullopt);
            ASSERT_ELSE(new_domain->tag == Tag::ProductValue, return nullopt;);
            auto product_value = term_cast<term::ProductValue>(new_domain);
            ASSERT_ELSE(product_value->type->tag == Tag::ProductType, return nullopt;);
            auto* product_type = term_cast<term::ProductType>(product_value->type);
            VAL_FROM_OPT_ELSE_RETURN(member_index,
                                     product_type->FindMemberIndex(projection->codomain), nullopt);
            return product_value->values[member_index];
        }
        case Tag::UnitLikeValue: {
            auto* unit_like_value = term_cast<term::UnitLikeValue>(term);
            VAL_FROM_OPT_ELSE_RETURN(type, EvaluateTerm(store, context, unit_like_value->type),
                                     nullopt);
            return store.MakeCanonical(term::UnitLikeValue(type));
        }
        case Tag::DeferredValue:
            // During evaluation we should not reach a deferred value. Deferred value can only occur
            // bound to a variable and we don't look up a variable if it contains a deferred value.
            ASSERT_ELSE(false, return nullopt;);
        case Tag::ProductValue: {
            auto* product_value = term_cast<term::ProductValue>(term);
            VAL_FROM_OPT_ELSE_RETURN(new_type_term,
                                     EvaluateTerm(store, context, product_value->type), nullopt);
            ASSERT_ELSE(new_type_term->tag == Tag::ProductType, return nullopt;);
            auto* new_type = term_cast<term::ProductType>(new_type_term);
            ASSERT_ELSE(new_type->members.size() == product_value->values.size(), return nullopt;);
            vector<TermPtr> new_values;
            for (int i = 0; i < new_type->members.size(); ++i) {
                VAL_FROM_OPT_ELSE_RETURN(new_cast_value,
                                         EvaluateTerm(store, context,
                                                      new term::Cast(product_value->values[i],
                                                                     new_type->members[i].type)),
                                         nullopt);
                new_values.push_back(new_cast_value);
            }
            return store.MakeCanonical(term::ProductValue(new_type, move(new_values)));
        }
        case Tag::FunctionType: {
            auto function_type = term_cast<term::FunctionType>(term);
            vector<TypeAndAvailability> new_parameter_types;
            for (auto p : function_type->parameter_types) {
                VAL_FROM_OPT_ELSE_RETURN(new_type, EvaluateTerm(store, context, p.type), nullopt);
                new_parameter_types.push_back(TypeAndAvailability{new_type, p.comptime});
            }
            VAL_FROM_OPT_ELSE_RETURN(
                new_result_type, EvaluateTerm(store, context, function_type->result_type), nullopt);
            return store.MakeCanonical(
                term::FunctionType(move(new_parameter_types), new_result_type));
        }
        case Tag::ProductType: {
            auto* product_type = term_cast<term::ProductType>(term);
            vector<term::TaggedType> new_members;
            for (auto m : product_type->members) {
                VAL_FROM_OPT_ELSE_RETURN(new_type, EvaluateTerm(store, context, m.type), nullopt);
                new_members.push_back(term::TaggedType{m.tag, new_type});
            }
            return store.MakeCanonical(term::ProductType(move(new_members)));
        }
        case Tag::Variable: {
            auto* variable = term_cast<term::Variable>(term);
            VAL_FROM_OPT_ELSE_UNREACHABLE_AND_RETURN(bound_value, context.LookUp(variable),
                                                     nullopt);
            if (bound_value->tag == Tag::DeferredValue) {
                auto* dv = term_cast<term::DeferredValue>(term);
                switch (dv->availability) {
                    case term::DeferredValue::Availability::Runtime:
                        UNREACHABLE;
                        return nullopt;
                    case term::DeferredValue::Availability::Comptime:
                        return variable;
                }
            }
            return EvaluateTerm(store, context, bound_value);
        }
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::SimpleTypeTerm:
            return term;
    }
}
optional<TermPtr> EvaluateTerm2(Store& store, const Context& context, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction:
            assert(false);  // TODO(BUG)
            return nullopt;
        case Tag::Application:
            assert(false);  // TODO(BUG)
            return nullopt;
        case Tag::Variable: {
            auto* variable = term_cast<term::Variable>(term);
            VAL_FROM_OPT_ELSE_RETURN(value, context.LookUp(variable), nullopt);
            if (value->tag != Tag::DeferredValue) {
                return value;
            }
            auto* deferred_value = term_cast<term::DeferredValue>(value);
            switch (deferred_value->role) {
                case term::DeferredValue::Availability::Runtime:  // This is probably an internal
                                                                  // error.
                    assert(false);
                    return nullopt;
                case term::DeferredValue::Availability::Comptime:  // This happens during
                                                                   // unification.
                    return variable;
            }
        }
        case Tag::ForAll:
        case Tag::Cast:
        case Tag::Projection:
        case Tag::UnitLikeValue:
        case Tag::DeferredValue:
        case Tag::ProductValue:
        case Tag::FunctionType:
        case Tag::ProductType:
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
            return EvaluateOrCompileTermSimpleAndSame(true, store, context, term);
    }
}
}  // namespace snl
