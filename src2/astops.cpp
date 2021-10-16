#include "astops.h"

#include "module.h"
#include "store.h"

namespace snl {

struct UnifyResult
{
    TermPtr resolved_pattern;
    BoundVariables new_bound_variables;
};

optional<UnifyResult> Unify(Store& store,
                            const Context& context,
                            TermPtr pattern,
                            TermPtr concrete,
                            const unordered_set<term::Variable const*>& variables_to_unify)
{
    assert(false);
    return nullopt;
}

FreeVariables GetFreeVariablesCore(Store& store, TermPtr term);

FreeVariables const* GetFreeVariables(Store& store, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::UnitLikeValue:
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType: {
            const static FreeVariables empty_fv;
            return &empty_fv;
        }
        default:
            break;
    }
    auto it = store.free_variables_of_terms.find(term);
    if (it == store.free_variables_of_terms.end()) {
        it = store.free_variables_of_terms
                 .insert(make_pair(term, store.MakeCanonical(GetFreeVariablesCore(store, term))))
                 .first;
    }
    return it->second;
}

FreeVariables GetFreeVariablesCore(Store& store, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction: {
            auto abstraction = term_cast<term::Abstraction>(term);
            auto fv = *GetFreeVariables(store, abstraction->body);
            std::unordered_set<term::Variable const*> vars_used_by_par_types;
            for (auto& p : abstraction->parameters) {
                auto fvs_of_expected_type =
                    keys_as_vector(GetFreeVariables(store, p.expected_type)->variables);
                vars_used_by_par_types.insert(BE(fvs_of_expected_type));
                fv.InsertWithFlowingToType(BE(fvs_of_expected_type));
            }
            for (auto& p : abstraction->parameters) {
                if (fv.DoesFlowIntoType(p.variable)) {
                    store.about_variables[p.variable].flows_into_type = true;
                }
                fv.EraseVariable(p.variable);
            }
            for (auto it = abstraction->bound_variables.rbegin();
                 it != abstraction->bound_variables.rend(); ++it) {
                bool flows_into_types = fv.DoesFlowIntoType(it->variable);
                if (flows_into_types) {
                    store.about_variables[it->variable].flows_into_type = true;
                }
                fv.EraseVariable(it->variable);
                auto* fv_of_value = GetFreeVariables(store, it->value);
                if (flows_into_types) {
                    auto keys = keys_as_vector(fv_of_value->variables);
                    fv.InsertWithFlowingToType(BE(keys));
                } else {
                    fv.InsertWithKeepingStronger(*fv_of_value);
                }
            }
            return fv;
        }
        case Tag::ForAll: {
            auto* forall = term_cast<term::ForAll>(term);
            auto fvs = GetFreeVariablesCore(store, forall->term);
            for (auto v : forall->variables) {
                bool erased = fvs.EraseVariable(v);
                assert(erased);
                (void)erased;
            }
            return fvs;
        }
        case Tag::Application: {
            auto application = term_cast<term::Application>(term);
            auto fv = *GetFreeVariables(store, application->function);
            for (auto& a : application->arguments) {
                fv.InsertWithKeepingStronger(*GetFreeVariables(store, a));
            }
            return fv;
        }
        case Tag::Variable: {
            FreeVariables fv;
            fv.variables[term_cast<term::Variable>(term)] =
                FreeVariables::VariableUsage::FlowsIntoValue;
            return fv;
        }
        case Tag::Projection: {
            auto projection = term_cast<term::Projection>(term);
            return *GetFreeVariables(store, projection->domain);
        }
        case Tag::Cast: {
            // TODO: Cast could be an internal function.
            // We should solve anyway that we can mark parameters comptime so their free variables
            // will be marked comptime, too.
            auto* cast = term_cast<term::Cast>(term);
            auto fvs = *GetFreeVariables(store, cast->subject);
            auto fv_target_type = GetFreeVariables(store, cast->target_type);
            fvs.InsertWithFlowingToType(BE(fv_target_type->variables));
            return fvs;
        }
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::UnitLikeValue:
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType: {
            assert(false);  // This should be caught in GetFreeVariables().
            const static FreeVariables empty_fv;
            return empty_fv;
        }
        case Tag::DeferredValue: {
            auto* dv = term_cast<term::DeferredValue>(term);
            return GetFreeVariables(store, dv->type)->CopyToTypeLevel();
        }
        case Tag::ProductValue: {
            auto product_value = term_cast<term::ProductValue>(term);
            FreeVariables fv;
            for (auto& v : product_value->values) {
                fv.InsertWithKeepingStronger(*GetFreeVariables(store, v));
            }
            return fv;
        }
        case Tag::FunctionType: {
            auto function_type = term_cast<term::FunctionType>(term);
            FreeVariables fv;
            for (auto p : function_type->parameter_types) {
                fv.InsertWithKeepingStronger(*GetFreeVariables(store, p));
            }
            fv.InsertWithKeepingStronger(*GetFreeVariables(store, function_type->result_type));
            return fv;
        }
        case Tag::ProductType: {
            auto product_type = term_cast<term::ProductType>(term);
            FreeVariables fv;
            for (auto m : product_type->members) {
                fv.InsertWithKeepingStronger(*GetFreeVariables(store, m.type));
            }
            return fv;
        }
    }
}

optional<TermPtr> EvaluateTerm(Store& store, const Context& context, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction:
            <#code #> break;
        case Tag::ForAll: {
            auto* tc = term_cast<term::ForAll>(term);
            auto new_term = EvaluateTerm(store, context, tc->term);
            if (!new_term) {
                return nullopt;
            }
            return store.MakeCanonical(term::ForAll(make_copy(tc->variables), *new_term));
        }
        case Tag::Application:
            <#code #> break;
        case Tag::Projection: {
            // TODO can we, are we allowed to skip evaluating the whole Projection if the domain is
            // already a ProductType?
            auto* projection = term_cast<term::Projection>(term);
            auto evaluated_domain = EvaluateTerm(store, context, projection->domain);
            if (!evaluated_domain) {
                return nullopt;
            }
            auto m_domain_type = InferTypeOfTerm(store, context, *evaluated_domain);
            if (!m_domain_type) {
                return nullopt;
            }
            auto domain_type = *m_domain_type;
            if (domain_type->tag != Tag::ProductType) {
                return nullopt;
            }
            auto product_type = term_cast<term::ProductType>(domain_type);
            auto member_index = product_type->FindMemberIndex(projection->codomain);
            if (!member_index) {
                return nullopt;
            }
            ASSERT_ELSE((*evaluated_domain)->tag == Tag::ProductValue, return nullopt;);
            auto product_value = term_cast<term::ProductValue>(*evaluated_domain);
            return product_value->values[*member_index];
        }
        case term::Tag::Cast: {
            auto* cast = term_cast<term::Cast>(term);
            auto new_subject = EvaluateTerm(store, context, cast->subject);
            if (!new_subject) {
                return nullopt;
            }
            auto new_target_type = EvaluateTerm(store, context, cast->target_type);
            if (!new_target_type) {
                return nullopt;
            }
            auto subject_type = InferTypeOfTerm(store, context, *new_subject);
            if (!subject_type) {
                return nullopt;
            }
            if (new_target_type == subject_type) {
                return *new_subject;
            }
            assert(false);  // TODO have no idea how to cast.
            return nullopt;
        }
        case Tag::UnitLikeValue: {
            auto* unit_like_value = term_cast<term::UnitLikeValue>(term);
            auto type = InferTypeOfTerm(store, context, term);
            if (!type) {
                return nullopt;
            }
            return store.MakeCanonical(term::UnitLikeValue(*type));
        }
        case Tag::DeferredValue:
            assert(false);  // I'm not sure if we ever allowed to evaluate this term.
            return nullopt;
        case Tag::ProductValue: {
            auto* pv = term_cast<term::ProductValue>(term);
            auto type = InferTypeOfTerm(store, context, term);
            if (!type) {
                return nullopt;
            }
            vector<TermPtr> new_values;
            for (auto v : pv->values) {
                auto new_value = EvaluateTerm(store, context, v);
                if (!new_value) {
                    return nullopt;
                }
                new_values.push_back(*new_value);
            }
            return store.MakeCanonical(term::ProductValue(*type, move(new_values)));
        }
        case Tag::FunctionType: {
            auto* tc = term_cast<term::FunctionType>(term);
            vector<TermPtr> new_parameter_types;
            for (auto pt : tc->parameter_types) {
                auto e = EvaluateTerm(store, context, pt);
                if (!e) {
                    return nullopt;
                }
                new_parameter_types.push_back(*e);
            }
            auto new_result_type = EvaluateTerm(store, context, tc->result_type);
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
                auto new_type = EvaluateTerm(store, context, m.type);
                if (!new_type) {
                    return nullopt;
                }
                new_members.push_back(term::TaggedType{m.tag, *new_type});
            }
            return store.MakeCanonical(term::ProductType(move(new_members)));
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
    }
}

optional<TermPtr> InferTypeOfTerm(Store& store, const Context& context, TermPtr term);

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
            // TODO can we, are we allowed to skip compiling the whole Projection if the domain is
            // already a ProductType? Then the projection can be executed here.
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

optional<TermPtr> InferTypeOfTermCore(Store& store, const Context& context, TermPtr term);
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
            auto m_domain_type = InferTypeOfTerm(store, context, projection->domain);
            if (!m_domain_type) {
                return nullopt;
            }
            auto domain_type = *m_domain_type;
            if (domain_type->tag != Tag::ProductType) {
                return nullopt;
            }
            auto product_type = term_cast<term::ProductType>(domain_type);
            auto m_tt = product_type->FindMember(projection->codomain);
            if (!m_tt) {
                return nullopt;
            }
            auto& tt = **m_tt;
            return tt.type;
        }
        case Tag::StringLiteral:
            return store.string_literal_type;
        case Tag::NumericLiteral:
            return store.numeric_literal_type;
        case Tag::UnitLikeValue:
            return EvaluateTerm(store, context, term_cast<term::UnitLikeValue>(term)->type);
        case Tag::DeferredValue:
            return EvaluateTerm(store, context, term_cast<term::DeferredValue>(term)->type);
        case Tag::ProductValue:
            return EvaluateTerm(store, context, term_cast<term::ProductValue>(term)->type);
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

/*
// Compute and return the actual value of the term.
optional<TermPtr> EvaluateTerm(const EvalContext& ec, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction: {
            auto* fv = GetFreeVariables(ec.store, term);

            Context inner_context(&ec.context);
            auto inner_ec = ec.DuplicateWithDifferentContext(inner_context);

            for (auto [k, v] : fv->variables) {
                // All free variables of this term must be bound.
                assert(ec.context.LookUp(k));
            }
            auto abstraction = term_cast<term::Abstraction>(term);
            for (auto& bv : abstraction->bound_variables) {
                // if bv flows into a type we need to evaluate it fully, otherwise, type only.
                optional<TermPtr> evaluated_bv;
                if (ec.store.DoesVariableFlowIntoType(bv.variable)) {
                    evaluated_bv = EvaluateTerm(inner_ec.DuplicateWithEvalValues(), bv.value);
                } else {
                    evaluated_bv = EvaluateTerm(inner_ec, bv.value);
                }
                if (!evaluated_bv) {
                    return nullopt;
                }
                inner_context.Bind(bv.variable, *evaluated_bv);
            }

            // Now come the parameters
            if (abstraction->parameters.empty()) {
                // No parameters: compute and return the result.
                return EvaluateTerm(inner_ec, abstraction->body);
            }

            auto yet_unbound_parameter_type_variables =
                unordered_set<term::Variable const*>(BE(abstraction->parameter_type_variables));
            for (auto& p : abstraction->parameters) {
                bool p_flows_into_type = ec.store.DoesVariableFlowIntoType(p.variable);
                auto fvs_of_expected_type = GetFreeVariables(ec.store, p.expected_type);
                // The yet unbound implicit variables now bound to an unspecified comptime
                // value.
                for (auto [k, v] : fvs_of_expected_type->variables) {
                    assert(v == FreeVariables::VariableUsage::FlowsIntoType);
                    if (yet_unbound_parameter_type_variables.count(k) > 0) {
                        inner_context.Bind(k, ec.store.comptime_value);
                        yet_unbound_parameter_type_variables.erase(k);
                        // Create a new, implicit variable, the type of k.
                        auto type_of_k = ec.store.MakeNewVariable();
                        ec.store.about_variables[k] = AboutVariable{type_of_k, true};
                        inner_context.Bind(type_of_k, ec.store.comptime_value);
                        ec.store.about_variables[type_of_k] =
                            AboutVariable{ec.store.type_of_types, true};
                    }
                }
                // Evaluate expected type
                auto evaluated_expected_type =
                    EvaluateTerm(inner_ec.DuplicateWithEvalValues(), p.expected_type);
                if (!evaluated_expected_type) {
                    return nullopt;
                }

                ec.store.about_variables[p.expected_type].type = evaluated_expected_type;
                // We mark this as comptime value if inner_ec.eval_values because we are at
                // compile time now.
                inner_context.Bind(p.variable, inner_ec.eval_values || p_flows_into_type
                                                   ? ec.store.comptime_value
                                                   : ec.store.runtime_value);
            }

            auto evaluated_body = EvaluateTerm(inner_ec, abstraction->body);
            if (!evaluated_body) {
                return nullptr;
            }

            // Might return the same if no change.
            return ec.store.MakeCanonical(term::Abstraction(
                ec.store, move(new_parameters), move(new_bound_variables), *evaluated_body));

        } break;
        case Tag::Application: {
            auto application = term_cast<term::Application>(term);
            vector<TermPtr> new_args;
            bool has_unbound_variables = false;
            for (auto arg : application->arguments) {
                if (auto evaluated_arg = EvaluateTerm(ec, arg)) {
                    new_args.push_back(*evaluated_arg);
                    if (ec.allow_unbound_variables && HasUnboundVariables(*evaluated_arg)) {
                        has_unbound_variables = true;
                    }
                } else {
                    return nullopt;
                }
            }
            if (has_unbound_variables) {
                return ec.store.MakeCanonical(term::Application());
            }
            return nullopt;
#if 0
            // Evaluate the arguments and compute and return the result.
            auto q =
            vector<TermPtr> evaluated_args;
            for (auto arg : q->arguments) {
                Context temporary_context(ec.context);
                Context inner_context(ec.context);
                auto inner_ec = ec.MakeInner(inner_context);
                if (auto evaluated_arg = EvaluateTerm(inner_ec, arg)) {
                    evaluated_args.push_back(*evaluated_arg);
                } else {
                    return nullopt;
                }
            }
            Context inner_context(ec.context);
            auto inner_ec = ec.MakeInner(inner_context);
            return EvaluateTerm(inner_ec, q->function);
#endif
        } break;
        case Tag::Variable: {
            // Look up the variable and evaluate it.
            auto variable = term_cast<term::Variable>(term);
            if (auto contents = ec.context.LookUp(variable)) {
                return EvaluateTerm(ec, *contents);
            } else {
                return nullopt;
            }
        } break;
        case Tag::Projection: {
            // Perform the projection and evaluate it.
            auto q = term_cast<term::Projection>(term);
            if (auto evaluated_domain_ = EvaluateTerm(ec, q->domain)) {
                auto evaluated_domain = *evaluated_domain_;
                if (evaluated_domain->tag == term::Tag::ProductValue) {
                    auto product_value = term_cast<term::ProductValue>(evaluated_domain);
                    auto product_type = term_cast<term::ProductType>(product_value->type);
                    optional<int> ix;
                    for (auto i = 0; i < product_type->members.size(); ++i) {
                        if (product_type->members[i].tag == q->codomain) {
                            ix = i;
                            break;
                        }
                    }
                    if (ix) {
                        return product_value->values[*ix];
                    } else {
                        printf("Projection tag not found.\n");
                        return nullopt;
                    }
                } else {
                    printf("Projection's argument is not a product type.\n");
                    return nullopt;
                }
            } else {
                return nullopt;
            }
        } break;
        // Irreducible terms.
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
            return term;
        case Tag::FunctionType: {
            // It's terms must be resolved.
            auto q = term_cast<term::FunctionType>(term);
            vector<TermPtr> resolved_terms;
            for (auto t : q->operand_types) {
                if (auto w = EvaluateTerm(ec, t)) {
                    resolved_terms.push_back(*w);
                } else {
                    return nullopt;
                }
            }
            if (resolved_terms == q->operand_types) {
                return term;
            } else {
                return ec.store.MakeCanonical(term::FunctionType(move(resolved_terms)));
            }
        } break;
        case Tag::ProductType: {
            auto q = term_cast<term::ProductType>(term);
            vector<term::TaggedType> new_members;
            for (auto& m : q->members) {
                if (auto w = EvaluateTerm(ec, m.type)) {
                    new_members.push_back(term::TaggedType{m.tag, *w});
                } else {
                    return nullopt;
                }
            }
            if (new_members == q->members) {
                return term;
            } else {
                return ec.store.MakeCanonical(term::ProductType(move(new_members)));
            }
        } break;
        case Tag::ProductValue: {
            auto q = term_cast<term::ProductValue>(term);
            auto new_type = EvaluateTerm(ec, q->type);
            if (!new_type || (*new_type)->tag != Tag::ProductType) {
                return nullopt;
            }
            vector<TermPtr> new_values;
            for (auto v : q->values) {
                auto w = EvaluateTerm(ec, v);
                if (!w) {
                    return nullopt;
                }
                new_values.push_back(*w);
            }
            if (*new_type == q->type && new_values == q->values) {
                return term;
            }
            return ec.store.MakeCanonical(
                term::ProductValue(term_cast<term::ProductType>(*new_type), move(new_values)));
        } break;
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
            return term;
        case Tag::RuntimeValue: {
            auto runtime_value = term_cast<term::RuntimeValue>(term);
            if (auto evaluated_type = EvaluateTerm(ec, runtime_value->type)) {
                if (*evaluated_type == runtime_value->type) {
                    return term;
                } else {
                    return ec.store.MakeCanonical(term::RuntimeValue(*evaluated_type));
                }
            } else {
                return nullopt;
            }
        }
        case Tag::UnitLikeValue: {
            // I'm not sure it makes sense havin a UnitLikeValue without a final, concrete type.
            if (auto evaluated_type = EvaluateTerm(ec.DuplicateWithEvalValues(), term->type)) {
                if (*evaluated_type == term->type) {
                    return term;
                } else {
                    ec.store.MakeCanonical(term::UnitLikeValue(*evaluated_type));
                }
            } else {
                return nullopt;
            }
        }
    }
    assert(false);
    return nullopt;
}
*/
/*
// Return a new term, equivalent to the term parameter, with its type resolved to a concrete,
// normal (irreducible) type, without InferredType terms.
optional<TermPtr> ResolveType(Store& store,
                                    const Context& context,
                                    TermPtr term)
{
    // 1. Evaluate term->type but don't fail on unbounded variables.
    //    T1 = eval-without-failing-on-unbounded term->type
    // 2. If it's a type in normal form, we're done, return it
    // 3. Otherwise it contains unbounded variables: evaluate the term for it's type.
    //    T2 = eval-type term
    // 4. Fail on failure. Otherwise we have a type in normal form. Unify it with T1:
    //    (T3, free-variables-in-T1) = unify-right-to-left T1 T2
    // 5. Return T3 and add the free variables to the input context
    auto resolved_type = EvaluateTerm(store, context, term->type);
    switch (term->tag) {
        case Tag::Abstraction: {
            auto q = term_cast<term::Abstraction>(term);
            // Types of abstraction can be resolved only if there are no parameters.
            if (!q->parameters.empty()) {
                printf("ResolveTypes: Abstraction with parameters can't be resolved.");
                return nullopt;
            }
            // Resolve types of bound variables
            Context inner_context(context);
            for (auto& v : q->bound_variables) {
                // this is not good, ResolveType returns a type but we need a value here.
                if (auto w = ResolveType(store, inner_context, v.value)) {
                    inner_context.variables[v.name] = *w;
                } else {
                    return nullopt;
                }
            }
            return ResolveType(store, inner_context, q->body);
        } break;
        case Tag::Application: {
            auto q = term_cast<term::Application>(term);
            // TODO eval function first then args then call?
            auto evaluated_function = ComptimeEval(store, context, q->function);
            vector<TermPtr> evaluated_args;
            for (auto arg : q->arguments) {
                evaluated_args.push_back(ComptimeEval(store, context, arg));
            }
        } break;
        case Tag::Variable:
            <#code #> break;
        case Tag::Projection:
            <#code #> break;
        case Tag::StringLiteral:
            <#code #> break;
        case Tag::NumericLiteral:
            <#code #> break;
        case Tag::TypeOfTypes:
            <#code #> break;
        case Tag::UnitType:
            <#code #> break;
        case Tag::BottomType:
            <#code #> break;
        case Tag::TopType:
            <#code #> break;
        case Tag::InferredType:
            <#code #> break;
        case Tag::FunctionType:
            <#code #> break;
        case Tag::StringLiteralType:
            <#code #> break;
        case Tag::NumericLiteralType:
            <#code #> break;
    }
}
*/
/*
// Return resolved pattern (which is usually identical to `concrete`)
optional<TermPtr> UnifyAndInferLeftTypes(Store& store,
                                               const Context& context,
                                               TermPtr pattern,
                                               TermPtr concrete,
                                               BoundVariables& new_bound_variables)
{
    using namespace term;
    if (!IsTypeInNormalForm(concrete)) {
        printf("UnifyAndInferLeftTypes: Concrete type not type in normal form.\n");
        return nullopt;
    }
    if (pattern->type != store.type_of_types) {
        printf("UnifyAndInferLeftTypes: pattern's type is not type-of-types.\n");
        return nullopt;
    }
    if (auto evaluated_pattern = ComptimeEval(context, pattern)) {
        pattern = *evaluated_pattern;
    } else {
        return nullopt;
    }
    for (bool allow_inferred_type : {true, false}) {
        switch (pattern->tag) {
            case Tag::Abstraction:
            case Tag::Application:
            case Tag::Variable:
            case Tag::Projection:
            case Tag::StringLiteral:
            case Tag::NumericLiteral:
                printf("Invalid term in pattern type.\n");
                return nullopt;
                // Leaf types:
            case Tag::TypeOfTypes:
            case Tag::UnitType:
            case Tag::BottomType:
            case Tag::TopType:
            case Tag::StringLiteralType:
            case Tag::NumericLiteralType:
                return pattern == concrete ? make_optional(pattern) : nullopt;
            case Tag::InferredType: {
                assert(allow_inferred_type);
                auto pattern_as_inferred_type = term_cast<term::InferredType>(pattern);
                if (auto resolved_pattern =
                        context.LookUpInferredType(pattern_as_inferred_type->id)) {
                    // Try again with resolved pattern.
                    assert((*resolved_pattern)->tag != Tag::InferredType);
                    pattern = *resolved_pattern;
                    break;
                } else {
                    // Bind this inferred type variable to concrete.
                    new_bound_variables.inferred_types[pattern_as_inferred_type->id] = concrete;
                    return concrete;
                }
            }
            case Tag::FunctionType: {
                if (concrete->tag != Tag::FunctionType) {
                    return nullopt;
                }
                // Check parameters of the function type.
                auto pattern_as_function_type = term_cast<term::FunctionType>(pattern);
                auto concrete_as_function_type = term_cast<term::FunctionType>(concrete);
                auto n_terms = pattern_as_function_type->terms.size();
                if (n_terms != concrete_as_function_type->terms.size()) {
                    return nullopt;
                }
                vector<TermPtr> resolved_terms;
                resolved_terms.reserve(n_terms);
                for (auto i = 0; i < n_terms; ++i) {
                    if (auto resolved_term = UnifyAndInferLeftTypes(
                            store, context, pattern_as_function_type->terms[i],
                            concrete_as_function_type->terms[i], new_bound_variables)) {
                        resolved_terms.push_back(*resolved_term);
                    } else {
                        return nullopt;
                    }
                }
                if (resolved_terms == concrete_as_function_type->terms) {
                    return concrete;
                }
                if (resolved_terms == pattern_as_function_type->terms) {
                    return pattern;
                }
                return store.MakeCanonical(FunctionType{move(resolved_terms)});
            }
        }
    }
    assert(false);
    return nullopt;
}

optional<TermPtr> UnifyAndInferLeftTypes(Store& store,
                                               Context& context,
                                               TermPtr pattern,
                                               TermPtr concrete)
{
    BoundVariables new_bound_variables;
    if (auto resolved_pattern =
            UnifyAndInferLeftTypes(store, context, pattern, concrete, new_bound_variables)) {
        context.append(move(new_bound_variables));
        return resolved_pattern;
    } else {
        return nullopt;
    }
}

std::optional<TermPtr> ApplyArgumentsToAbstraction(
    Store& store,
    Context const* const parent_bound_variables,
    term::Abstraction const* abstraction,
    const vector<TermPtr>& args)
{
    auto n_args = args.size();
    auto n_pars = abstraction->parameters.size();
    // First, apply as many args as possible.
    auto n_args_apply = std::min(n_args, n_pars);
    auto new_parameters = vector<term::Parameter>(abstraction->parameters.begin(),
                                                  abstraction->parameters.end() - n_args_apply);
    auto new_bound_variables = abstraction->bound_variables;
    Context context(parent_bound_variables);

    for (auto arg_ix = 0; arg_ix < n_args_apply; ++arg_ix) {
        auto arg = args[arg_ix];
        if (!term::IsTypeInNormalForm(arg->type)) {
            printf("arg type not in normal form\n");
            return nullopt;
        }
        auto par_ix = n_pars - n_args_apply + arg_ix;
        auto& par = abstraction->parameters[par_ix];
        optional<term::BoundVariable> bound_variable;
        if (par.type_annotation) {
            if (auto resolved_par_type =
                    UnifyAndInferLeftTypes(store, context, *par.type_annotation, arg->type)) {
                // use resolved type

                // otherwise I don't know how to cast to subtype:
                assert(*resolved_par_type == arg->type);

                // context is also updated with resolved inferred types
                bound_variable = term::BoundVariable{par.name, arg};
            } else {
                printf("can't unify types");
                return nullopt;
            }
        } else {
            // Just use the argument type as it is.
            bound_variable = term::BoundVariable{par.name, arg};
        }

        new_bound_variables.emplace_back(move(bound_variable.value()));
    }
    auto new_abstraction = store.MakeCanonical(term::Abstraction(
        store, move(new_parameters), move(new_bound_variables), abstraction->body));

    auto n_args_remain = n_args - n_args_apply;
    if (n_args_remain == 0) {
        // new_abstraction can be actual abstraction with free variables or "value-like"
        // abstraction with zero free variables (all free variables in the body are bound).
        return new_abstraction;
    }
    // There are remaining arguments.
    auto new_arguments = vector<TermPtr>(args.begin() + n_args_apply, args.end());
    return store.MakeCanonical(
        term::Application(new_abstraction->ResultType(), new_abstraction, move(new_arguments)));
}
*/

}  // namespace snl
