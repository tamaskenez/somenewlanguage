#include "evaluateorcompileterm.h"

#include "astops.h"
#include "context.h"
#include "store.h"

namespace snl {

optional<TermPtr> EvaluateOrCompileTerm(bool eval,
                                        Store& store,
                                        const Context& context,
                                        TermPtr term)
{
    return eval ? EvaluateTerm(store, context, term) : CompileTerm(store, context, term);
}

optional<TermPtr> EvaluateOrCompileTermSimpleAndSame(bool eval,
                                                     Store& store,
                                                     const Context& context,
                                                     TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case term::Tag::Abstraction:
            assert(false);
            return nullopt;

        case term::Tag::Application: {
            auto* application = term_cast<term::Application>(term);
            MOVE_FROM_OPT_ELSE_RETURN(
                argument_types, InferTypeOfTerms(store, context, application->arguments), nullopt);
            MOVE_FROM_OPT_ELSE_RETURN(
                callee_types,
                InferCalleeTypesa(store, context, application->function, argument_types), nullopt);

            // TODO: If we've resolved some for-all variables, make record of this at the
            // abstraction. On the second compile pass (?), instead of the for-all constructs we
            // should compile exactly the needed concrete abstractions. Here we should
            // explicitly ask for our configuration of resolved variables, which may be partial.

            VAL_FROM_OPT_ELSE_RETURN(
                processed_function,
                EvaluateOrCompileTerm(eval, store, context, application->function), nullopt);

            vector<TermPtr> cast_arguments;
            int n_args = application->arguments.size();
            assert(argument_types.size() == n_args);
            assert(callee_types.bound_parameter_types.size() == n_args);
            for (int i = 0; i < n_args; ++i) {
                auto arg_type = argument_types[i];
                auto par_type = callee_types.bound_parameter_types[i];
                auto arg = application->arguments[i];
                VAL_FROM_OPT_ELSE_RETURN(
                    processed_arg,
                    EvaluateOrCompileTerm(eval, store, context,
                                          arg_type == par_type
                                              ? arg
                                              : store.MakeCanonical(term::Cast(arg, par_type))),
                    nullopt);
                cast_arguments.push_back(processed_arg);
            }
            // If there are no parameters left, this term compiles into a function call.
            // If there are parameters left this term compiles into an abstraction, optionally
            // No for-all context, it was a concrete function type.
            if (callee_types.remaining_parameter_types.empty()) {
                // TODO: inline compiled_function if applicable:
                // Instead of Application calling Abstraction, create a new Abstraction without
                // parameter, the arguments passed in the bound variables of the Abstraction.
                assert(callee_types.remaining_forall_variables
                           .empty());  // All of them must have been bound.
                if (eval) {
                    // We actually need to evaluate the function with the arguments.
                    if (processed_function->tag != Tag::Abstraction) {
                        return nullopt;
                    }
                    auto* abstraction = term_cast<term::Abstraction>(processed_function);
                    vector<term::BoundVariable> new_bound_variables =
                        abstraction->bound_variables;  // Copy.
                    assert(abstraction->parameters.size() == n_args);
                    Context inner_context(&context);
                    // We need the inner context after all parameters have been bound.
                    // TODO(BUG) we need it from InferCalleeTypes.
                    for (auto& bv : abstraction->bound_variables) {
                        inner_context.Bind(bv.variable,
                                           bv.value);  // value expeced to be evaluated because the
                                                       // whole abstraction is.
                    }
                    return EvaluateTerm(store, inner_context, abstraction->body);
                } else {
                    return store.MakeCanonical(
                        term::Application(processed_function, move(cast_arguments)));
                }
            }

            // TODO: inline compiled_function if applicable:
            // Instead of returning an Abstraction with body of an Application calling the
            // original Abstraction, create a new Abstraction with the remaining parameters, the
            // arguments passed in the bound variables of the Abstraction.
            // TODO(BUG) we need the bound forall variables as bound variables here.
            // TODO(BUG) this part must be made compatible with the evaluate branch.
            vector<term::BoundVariable> new_bound_variables;
            vector<TermPtr> new_arguments;
            for (int i = 0; i < n_args; ++i) {
                auto var = store.MakeNewVariable();
                new_bound_variables.push_back(term::BoundVariable{var, cast_arguments[i]});
                new_arguments.push_back(var);
            }
            vector<term::Parameter> new_parameters;
            for (auto rpt : callee_types.remaining_parameter_types) {
                auto var = store.MakeNewVariable();
                new_parameters.push_back(term::Parameter{var, rpt});
                new_arguments.push_back(var);
            }
            auto new_body =
                store.MakeCanonical(term::Application(processed_function, move(new_arguments)));
            auto new_abstraction = store.MakeCanonical(
                term::Abstraction(move(new_bound_variables), move(new_parameters), new_body));
            if (callee_types.remaining_forall_variables.empty()) {
                return new_abstraction;
            } else {
                return store.MakeCanonical(
                    term::ForAll(move(callee_types.remaining_forall_variables), new_abstraction));
            }
        }

        case term::Tag::ForAll: {
            auto* tc = term_cast<term::ForAll>(term);
            VAL_FROM_OPT_ELSE_RETURN(
                new_term, EvaluateOrCompileTerm(eval, store, context, tc->term), nullopt);
            return store.MakeCanonical(term::ForAll(make_copy(tc->variables), new_term));
        }
        case term::Tag::Cast: {
            auto* cast = term_cast<term::Cast>(term);
            VAL_FROM_OPT_ELSE_RETURN(
                new_subject, EvaluateOrCompileTerm(eval, store, context, cast->subject), nullopt);
            VAL_FROM_OPT_ELSE_RETURN(new_target_type,
                                     EvaluateTerm(store, context, cast->target_type), nullopt);
            if (eval) {
                VAL_FROM_OPT_ELSE_RETURN(subject_type, InferTypeOfTerm(store, context, new_subject),
                                         nullopt);
                if (new_target_type == subject_type) {
                    return new_subject;
                }
                assert(false);  // TODO have no idea how to cast. We should call the built-in `cast`
                                // function.
                return nullopt;
            } else {
                return store.MakeCanonical(term::Cast(new_subject, new_target_type));
            }
        }
        case term::Tag::Projection: {
            auto* projection = term_cast<term::Projection>(term);
            VAL_FROM_OPT_ELSE_RETURN(
                new_domain, EvaluateOrCompileTerm(eval, store, context, projection->domain),
                nullopt);
            VAL_FROM_OPT_ELSE_RETURN(domain_type, InferTypeOfTerm(store, context, new_domain),
                                     nullopt);
            if (domain_type->tag != Tag::ProductType) {
                return nullopt;
            }
            auto product_type = term_cast<term::ProductType>(domain_type);
            VAL_FROM_OPT_ELSE_RETURN(member_index,
                                     product_type->FindMemberIndex(projection->codomain), nullopt);
            if (eval) {
                ASSERT_ELSE(new_domain->tag == Tag::ProductValue, return nullopt;);
                auto product_value = term_cast<term::ProductValue>(new_domain);
                return product_value->values[member_index];
            } else {
                return store.MakeCanonical(
                    term::Projection(new_domain, make_copy(projection->codomain)));
            }
        }
        case term::Tag::UnitLikeValue: {
            auto* unit_like_value = term_cast<term::UnitLikeValue>(term);
            VAL_FROM_OPT_ELSE_RETURN(type, InferTypeOfTerm(store, context, term), nullopt);
            return store.MakeCanonical(term::UnitLikeValue(type));
        }
        case term::Tag::DeferredValue:
            // During compilation we never look up a variable.
            // During evaluation we look up a variable only if it's not bound to a deferred value.
            assert(false);
            return nullopt;
        case term::Tag::ProductValue: {
            auto* pv = term_cast<term::ProductValue>(term);
            vector<TermPtr> new_values;
            for (auto v : pv->values) {
                VAL_FROM_OPT_ELSE_RETURN(new_value, EvaluateOrCompileTerm(eval, store, context, v),
                                         nullopt);
                new_values.push_back(new_value);
            }
            VAL_FROM_OPT_ELSE_RETURN(type, InferTypeOfTerm(store, context, term), nullopt);
            return store.MakeCanonical(term::ProductValue(type, move(new_values)));
        }
        case term::Tag::FunctionType: {
            auto function_type = term_cast<term::FunctionType>(term);
            vector<TermPtr> new_parameter_types;
            for (auto p : function_type->parameter_types) {
                VAL_FROM_OPT_ELSE_RETURN(new_parameter,
                                         EvaluateOrCompileTerm(eval, store, context, p), nullopt);
                new_parameter_types.push_back(new_parameter);
            }
            VAL_FROM_OPT_ELSE_RETURN(
                new_result_type,
                EvaluateOrCompileTerm(eval, store, context, function_type->result_type), nullopt);
            return store.MakeCanonical(
                term::FunctionType(move(new_parameter_types), new_result_type));
        }
        case term::Tag::ProductType: {
            auto* product_type = term_cast<term::ProductType>(term);
            vector<term::TaggedType> new_members;
            for (auto m : product_type->members) {
                VAL_FROM_OPT_ELSE_RETURN(
                    new_type, EvaluateOrCompileTerm(eval, store, context, m.type), nullopt);
                new_members.push_back(term::TaggedType{m.tag, new_type});
            }
            return store.MakeCanonical(term::ProductType(move(new_members)));
        }

        case term::Tag::Variable:
        case term::Tag::StringLiteral:
        case term::Tag::NumericLiteral:
        case term::Tag::TypeOfTypes:
        case term::Tag::UnitType:
        case term::Tag::BottomType:
        case term::Tag::TopType:
        case term::Tag::StringLiteralType:
        case term::Tag::NumericLiteralType:
            return term;
    }
}
}  // namespace snl
