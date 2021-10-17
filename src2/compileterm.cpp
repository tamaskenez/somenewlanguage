#include "astops.h"

#include "freevariablesofterm.h"
#include "store.h"

namespace snl {

// Product a term which can be evaluated to a value if all free variables are bound.
optional<TermPtr> CompileTerm(Store& store, const Context& context, TermPtr term)
{
    // All free variables of this term must be bound.
    auto* fv = GetFreeVariables(store, term);
    for (auto [k, v] : fv->variables) {
        ASSERT_ELSE(context.LookUp(k), return nullopt;);
    }

    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction: {
            Context inner_context(&context);

            auto* abstraction = term_cast<term::Abstraction>(term);

            GetFreeVariables(store,
                             term);  // Making sure all free variable evaluation related caches
                                     // are initialized.

            // Evaluate bound variables.
            for (auto& bv : abstraction->bound_variables) {
                // if bv flows into a type we need to evaluate it fully, otherwise, type only.
                optional<TermPtr> evaluated_bv;
                if (store.DoesVariableFlowIntoType(bv.variable)) {
                    evaluated_bv = EvaluateTerm(store, inner_context, bv.value);
                } else {
                    evaluated_bv = CompileTerm(store, inner_context, bv.value);
                }
                if (!evaluated_bv) {
                    return nullopt;
                }
                inner_context.Bind(bv.variable, *evaluated_bv);
            }

            if (abstraction->parameters.empty()) {
                // No parameters, compile the body into a value.
                return CompileTerm(store, inner_context, abstraction->body);
            }

            for (auto& p : abstraction->parameters) {
                ASSERT_ELSE(!store.DoesVariableFlowIntoType(p.variable), return nullopt;);
                auto evaluated_type = EvaluateTerm(store, inner_context, p.expected_type);
                if (!evaluated_type) {
                    return nullopt;
                }
                inner_context.Bind(p.variable,
                                   store.MakeCanonical(term::DeferredValue(
                                       *evaluated_type, term::DeferredValue::Role::Runtime)));
            }
            // Can be compiled into abstraction
            auto compiled_body = CompileTerm(store, inner_context, abstraction->body);
            if (!compiled_body) {
                return nullopt;
            }
            vector<term::BoundVariable> compiled_bound_variables;
            for (auto& bv : abstraction->bound_variables) {
                compiled_bound_variables.push_back(
                    term::BoundVariable{bv.variable, inner_context.LookUp(bv.variable).value()});
            }
            vector<term::Parameter> compiled_parameters;
            for (auto& p : abstraction->parameters) {
                compiled_parameters.push_back(
                    term::Parameter{p.variable, inner_context.LookUp(p.variable).value()});
            }
            return store.MakeCanonical(term::Abstraction(
                move(compiled_bound_variables), move(compiled_parameters), *compiled_body));
        }
        case Tag::ForAll: {
            auto* tc = term_cast<term::ForAll>(term);
            auto new_term = CompileTerm(store, context, tc->term);
            if (!new_term) {
                return nullopt;
            }
            return store.MakeCanonical(term::ForAll(make_copy(tc->variables), *new_term));
        }
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
        case Tag::Variable:
            return term;
        case Tag::DeferredValue:
            assert(false);  // I'm not sure if we ever allowed to compile this term.
            return nullopt;

        case Tag::Application: {
            auto* application = term_cast<term::Application>(term);
            auto m_argument_types = InferTypeOfTerms(store, context, application->arguments);
            if (!m_argument_types) {
                return nullopt;
            }
            auto& argument_types = *m_argument_types;
            auto callee_types =
                InferCalleeTypes(store, context, application->function, argument_types);
            if (!callee_types) {
                return nullopt;
            }
            // TODO: If we've resolved some for-all variables, make record of this at the
            // abstraction. On the second compile pass (?), instead of the for-all constructs we
            // should compile exactly the needed concrete abstractions. Here we should
            // explicitly ask for our configuration of resolved variables, which may be partial.

            auto compiled_function = CompileTerm(store, context, application->function);
            if (!compiled_function) {
                return nullopt;
            }

            vector<TermPtr> cast_arguments;
            int n_args = application->arguments.size();
            assert(argument_types.size() == n_args);
            assert(callee_types->bound_parameter_types.size() == n_args);
            for (int i = 0; i < n_args; ++i) {
                auto arg_type = argument_types[i];
                auto par_type = callee_types->bound_parameter_types[i];
                auto arg = application->arguments[i];
                cast_arguments.push_back(
                    arg_type == par_type ? arg : store.MakeCanonical(term::Cast(arg, par_type)));
            }
            // If there are no parameters left, this term compiles into a function call.
            // If there are parameters left this term compiles into an abstraction, optionally
            // No for-all context, it was a concrete function type.
            if (callee_types->remaining_parameter_types.empty()) {
                // TODO: inline compiled_function if applicable:
                // Instead of Application calling Abstraction, create a new Abstraction without
                // parameter, the arguments passed in the bound variables of the Abstraction.
                assert(callee_types->remaining_forall_variables
                           .empty());  // All of them must have been bound.
                return store.MakeCanonical(
                    term::Application(*compiled_function, move(cast_arguments)));
            }

            // TODO: inline compiled_function if applicable:
            // Instead of returning an Abstraction with body of an Application calling the
            // original Abstraction, create a new Abstraction with the remaining parameters, the
            // arguments passed in the bound variables of the Abstraction.
            vector<term::BoundVariable> new_bound_variables;
            vector<TermPtr> new_arguments;
            for (int i = 0; i < n_args; ++i) {
                auto var = store.MakeNewVariable();
                new_bound_variables.push_back(term::BoundVariable{var, cast_arguments[i]});
                new_arguments.push_back(var);
            }
            vector<term::Parameter> new_parameters;
            for (auto rpt : callee_types->remaining_parameter_types) {
                auto var = store.MakeNewVariable();
                new_parameters.push_back(term::Parameter{var, rpt});
                new_arguments.push_back(var);
            }
            auto new_body =
                store.MakeCanonical(term::Application(*compiled_function, move(new_arguments)));
            auto new_abstraction = store.MakeCanonical(
                term::Abstraction(move(new_bound_variables), move(new_parameters), new_body));
            if (callee_types->remaining_forall_variables.empty()) {
                return new_abstraction;
            } else {
                return store.MakeCanonical(
                    term::ForAll(move(callee_types->remaining_forall_variables), new_abstraction));
            }
        }
        case Tag::Projection: {
            auto* projection = term_cast<term::Projection>(term);
            auto compiled_domain = CompileTerm(store, context, projection->domain);
            if (!compiled_domain) {
                return nullopt;
            }
            auto m_domain_type = InferTypeOfTerm(store, context, *compiled_domain);
            if (!m_domain_type) {
                return nullopt;
            }
            auto domain_type = *m_domain_type;
            if (domain_type->tag != Tag::ProductType) {
                return nullopt;
            }
            auto product_type = term_cast<term::ProductType>(domain_type);
            if (!product_type->FindMemberIndex(projection->codomain)) {
                return nullopt;
            }
            return store.MakeCanonical(
                term::Projection(*compiled_domain, make_copy(projection->codomain)));
        }
        case Tag::Cast: {
            auto* cast = term_cast<term::Cast>(term);
            auto new_subject = CompileTerm(store, context, cast->subject);
            if (!new_subject) {
                return nullopt;
            }
            auto new_target_type = EvaluateTerm(store, context, cast->target_type);
            if (!new_target_type) {
                return nullopt;
            }
            return store.MakeCanonical(term::Cast(*new_subject, *new_target_type));
        }
        case Tag::UnitLikeValue: {
            auto* unit_like_value = term_cast<term::UnitLikeValue>(term);
            auto type = InferTypeOfTerm(store, context, term);
            if (!type) {
                return nullopt;
            }
            return store.MakeCanonical(term::UnitLikeValue(*type));
        }
        case Tag::ProductValue: {
            auto* pv = term_cast<term::ProductValue>(term);
            auto type = InferTypeOfTerm(store, context, term);
            if (!type) {
                return nullopt;
            }
            vector<TermPtr> new_values;
            for (auto v : pv->values) {
                auto new_value = CompileTerm(store, context, v);
                if (!new_value) {
                    return nullopt;
                }
                new_values.push_back(*new_value);
            }
            return store.MakeCanonical(term::ProductValue(*type, move(new_values)));
        }
        case Tag::FunctionType: {
            auto function_type = term_cast<term::FunctionType>(term);
            vector<TermPtr> new_parameter_types;
            for (auto p : function_type->parameter_types) {
                auto new_parameter = CompileTerm(store, context, p);
                if (!new_parameter) {
                    return nullopt;
                }
                new_parameter_types.push_back(*new_parameter);
            }
            auto new_result_type = CompileTerm(store, context, function_type->result_type);
            if (!new_result_type) {
                return nullopt;
            }
            return store.MakeCanonical(
                term::FunctionType(move(new_parameter_types), *new_result_type));
        }
        case Tag::ProductType: {
            auto* product_type = term_cast<term::ProductType>(term);
            vector<term::TaggedType> new_members;
            for (auto m : product_type->members) {
                auto new_type = CompileTerm(store, context, m.type);
                if (!new_type) {
                    return nullopt;
                }
                new_members.push_back(term::TaggedType{m.tag, *new_type});
            }
            return store.MakeCanonical(term::ProductType(move(new_members)));
        }
    }
}
}  // namespace snl
