#include "astops.h"

#include "evaluateorcompileterm.h"
#include "store.h"
#include "unify.h"

namespace snl {

optional<TermPtr> EvaluatePartialApplicationByEmbeddingOriginalAbstraction(
    Store& store,
    const Context& context,
    term::Application const* application,
    term::Abstraction const* abstraction)
{
    int n_args = application->arguments.size();
    int n_pars = abstraction->parameters.size();

    // We have something like this:
    //
    //     Appl (innerabstr x y z) (ax ay)
    //
    // We create this instead:
    //
    //     ForAll Z. Abstr zarg::Z
    //         Appl (innerabstr x y z) (eval(ax) eval(ay) zarg)
    //
    assert(n_args < n_pars);
    vector<TermPtr> inner_arguments;
    vector<Parameter> remaining_parameters;

    for (int i = 0; i < n_args; ++i) {
        VAL_FROM_OPT_ELSE_RETURN(evaluated_arg,
                                 EvaluateTerm(store, context, application->arguments[i]), nullopt);
        inner_arguments.push_back(evaluated_arg);
        // auto par = abstraction->parameters[i];
        // bool comptime = abstraction->forall_variables.count(par.variable)>0;
        // auto variable = store.MakeNewVariable(comptime);
        // bound_variables.push_back(BoundVariable{par.variable, evaluated_arg});
    }
    unordered_set<term::Variable const*> remaining_parameter_types;
    for (int i = n_args; i < n_pars; ++i) {
        auto par = abstraction->parameters[i];
        auto new_par_type_variable = store.MakeNewVariable(true);
        auto new_par_variable = store.MakeNewVariable(par.variable->comptime);
        inner_arguments.push_back(new_par_variable);
        remaining_parameters.push_back(Parameter(new_par_variable, new_par_type_variable));
        remaining_parameter_types.insert(new_par_type_variable);
    }

    unordered_set<term::Variable const*> forall_variables;

    auto inner_application =
        store.MakeCanonical(term::Application(abstraction, move(inner_arguments)));

    TermPtr body;
    MOVE_FROM_OPT_ELSE_RETURN(new_abstraction,
                              term::Abstraction::MakeAbstraction(
                                  store, move(remaining_parameter_types), vector<BoundVariable>(),
                                  move(remaining_parameters), inner_application),
                              nullopt);
    return store.MakeCanonical(move(new_abstraction));
}

optional<TermPtr> EvaluateApplication(Store& store,
                                      const Context& context,
                                      term::Application const* application)
{
    ASSERT_ELSE(!application->arguments.empty(), return nullopt;);
    // Investigate `application->function`
    VAL_FROM_OPT_ELSE_RETURN(evaluated_function,
                             EvaluateTerm(store, context, application->function), nullopt);
    switch (evaluated_function->tag) {
        case term::Tag::Abstraction: {
            auto* abstraction = term_cast<term::Abstraction>(evaluated_function);

            int n_args = application->arguments.size();
            int n_pars = abstraction->parameters.size();

            constexpr bool k_dont_embed_full_abstraction_when_currying = false;

            if (k_dont_embed_full_abstraction_when_currying && n_args < n_pars) {
                return EvaluatePartialApplicationByEmbeddingOriginalAbstraction(
                    store, context, application, abstraction);
            }

            unordered_set<term::Variable const*> forall_variables =
                abstraction->forall_variables;  // Copy.

            // Add bound variables to context. They're expected to be evaluated.
            Context inner_context(&context);
            vector<BoundVariable> bound_variables = abstraction->bound_variables;  // Copy.
            for (auto bv : abstraction->bound_variables) {
                inner_context.Bind(bv.variable, bv.value);
            }

            // Apply arguments to parameters.
            int n_applied_args = std::min(n_args, n_pars);
            for (int i = 0; i < n_applied_args; ++i) {
                VAL_FROM_OPT_ELSE_RETURN(
                    evaluated_arg, EvaluateTerm(store, inner_context, application->arguments[i]),
                    nullopt);
                auto par = abstraction->parameters[i];
                // par.variable can be contained in forall_variables (= comptime par) but we
                // don't care here since everything must be evaluated at this point.
                forall_variables.erase(par.variable);
                VAL_FROM_OPT_ELSE_RETURN(
                    arg_type, InferTypeOfTerm(store, inner_context, evaluated_arg), nullopt);
                MOVE_FROM_OPT_ELSE_RETURN(
                    unify_result,
                    Unify(store, inner_context, par.expected_type, arg_type, forall_variables),
                    nullopt);
                for (auto [var, val] : unify_result.new_bound_variables) {
                    forall_variables.erase(var);
                    inner_context.Bind(var, val);
                    bound_variables.push_back(BoundVariable{var, val});
                }
                bound_variables.push_back(BoundVariable{par.variable, evaluated_arg});
                inner_context.Bind(par.variable, evaluated_arg);
            }

            vector<TermPtr> remaining_arguments(application->arguments.begin() + n_applied_args,
                                                application->arguments.end());
            vector<Parameter> remaining_parameters(abstraction->parameters.begin() + n_applied_args,
                                                   abstraction->parameters.end());

            if (!remaining_parameters.empty()) {
                // Return new abstraction with remaining parameters.
                assert(remaining_arguments.empty());
                MOVE_FROM_OPT_ELSE_RETURN(new_abstraction,
                                          term::Abstraction::MakeAbstraction(
                                              store, move(forall_variables), move(bound_variables),
                                              move(remaining_parameters), abstraction->body),
                                          nullopt);
                return store.MakeCanonical(move(new_abstraction));
            }
            assert(forall_variables.empty());

            // Evaluate body.
            VAL_FROM_OPT_ELSE_RETURN(
                evaluated_body, EvaluateTerm(store, inner_context, abstraction->body), nullopt);

            if (!remaining_arguments.empty()) {
                return evaluated_body;
            }

            auto new_application =
                store.MakeCanonical(term::Application(evaluated_body, move(remaining_arguments)));
            return EvaluateTerm(store, context, evaluated_body);
        }
        case term::Tag::LetIns:
        case term::Tag::Application:
        case term::Tag::Variable:
        case term::Tag::CppTerm:
        case term::Tag::StringLiteral:
        case term::Tag::NumericLiteral:
        case term::Tag::UnitLikeValue:
        case term::Tag::DeferredValue:
        case term::Tag::ProductValue:
        case term::Tag::SimpleTypeTerm:
        case term::Tag::NamedType:
        case term::Tag::FunctionType:
        case term::Tag::TypeOfAbstraction:
        case term::Tag::ProductType:
            return nullopt;
    }
}

optional<TermPtr> EvaluateTerm(Store& store, const Context& context, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction:
            return term;
        case Tag::LetIns: {
            auto* letins = term_cast<term::LetIns>(term);
            Context inner_context(&context);
            for (auto& bv : letins->bound_variables) {
                VAL_FROM_OPT_ELSE_RETURN(evaluated_value,
                                         EvaluateTerm(store, inner_context, bv.value), nullopt);
                inner_context.Bind(bv.variable, evaluated_value);
            }
            return EvaluateTerm(store, inner_context, letins->body);
        }
        case Tag::Application: {
            auto* application = term_cast<term::Application>(term);
            return EvaluateApplication(store, context, application);
        }
        /*
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
        }*/
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
            Context inner_context(&context);
            for (auto v : function_type->forall_variables) {
                inner_context.Bind(v, store.comptime_value_comptime_type);
            }
            vector<TypeAndAvailability> new_parameter_types;
            for (auto p : function_type->parameter_types) {
                VAL_FROM_OPT_ELSE_RETURN(new_type, EvaluateTerm(store, context, p.type), nullopt);
                new_parameter_types.push_back(TypeAndAvailability{new_type, p.comptime_parameter});
                if (p.comptime_parameter) {
                    inner_context.Bind(*p.comptime_parameter,
                                       store.MakeCanonical(term::DeferredValue(
                                           new_type, term::DeferredValue::Availability::Comptime)));
                }
            }
            VAL_FROM_OPT_ELSE_RETURN(
                new_return_type, EvaluateTerm(store, context, function_type->return_type), nullopt);
            return store.MakeCanonical(
                term::FunctionType(make_copy(function_type->forall_variables),
                                   move(new_parameter_types), new_return_type));
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
#if 0
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
#endif
}  // namespace snl
