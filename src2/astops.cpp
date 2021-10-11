#include "astops.h"

#include "module.h"

namespace snl {

/*
optional<InitialPassError> InitialPass(const InitialPassContext& ipc, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction: {
            auto abstraction = term_cast<term::Abstraction>(term);
            // Abstraction is bound variables, parameters, body
            BoundVariablesWithParent inner_context(&ipc.context);
            auto inner_ipc = ipc.DuplicateWithDifferentContext(inner_context);
            for(auto&bv:abstraction->bound_variables){
            if(auto error = InitialPass(inner_ipc, bv.value)){return error; }
            inner_context.Bind(bv.variable, bv.value);
            }
            for(auto &p:abstraction->parameters){
            p.
            }
        } break;
        case term::Tag::Application:
            <#code #> break;
        case term::Tag::Variable:
            <#code #> break;
        case term::Tag::Projection:
            <#code #> break;
        case term::Tag::StringLiteral:
            <#code #> break;
        case term::Tag::NumericLiteral:
            <#code #> break;
        case term::Tag::UnitLikeValue:
            <#code #> break;
        case term::Tag::RuntimeValue:
            <#code #> break;
        case term::Tag::ComptimeValue:
            <#code #> break;
        case term::Tag::ProductValue:
            <#code #> break;
        case term::Tag::TypeOfTypes:
            <#code #> break;
        case term::Tag::UnitType:
            <#code #> break;
        case term::Tag::BottomType:
            <#code #> break;
        case term::Tag::TopType:
            <#code #> break;
        case term::Tag::FunctionType:
            <#code #> break;
        case term::Tag::ProductType:
            <#code #> break;
        case term::Tag::StringLiteralType:
            <#code #> break;
        case term::Tag::NumericLiteralType:
            <#code #> break;
    }
}
*/
struct UnifyResult
{
    TermPtr resolved_pattern;
    BoundVariables new_bound_variables;
};

optional<UnifyResult> Unify(Store& store,
                            const BoundVariablesWithParent& context,
                            TermPtr pattern,
                            TermPtr concrete)
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
            for (auto& itp : abstraction->implicit_type_parameters) {
                assert(vars_used_by_par_types.count(itp) > 0);
                fv.EraseVariable(itp);
            }
            return fv;
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
        case term::Tag::RuntimeValue:
        case term::Tag::ComptimeValue: {
            assert(false);  // Not sure this makes sense (RuntimeValue & ComptimeValue).
            auto* value_term = static_cast<ValueTerm const*>(term);
            assert(value_term);
            return GetFreeVariables(store, value_term->type)->CopyToTypeLevel();
        }
        case term::Tag::ProductValue: {
            auto product_value = term_cast<term::ProductValue>(term);
            FreeVariables fv;
            for (auto& v : product_value->values) {
                fv.InsertWithKeepingStronger(*GetFreeVariables(store, v));
            }
            return fv;
        }
        case term::Tag::FunctionType: {
            auto function_type = term_cast<term::FunctionType>(term);
            FreeVariables fv;
            for (auto o : function_type->operand_types) {
                fv.InsertWithKeepingStronger(*GetFreeVariables(store, o));
            }
            return fv;
        }
        case term::Tag::ProductType: {
            auto product_type = term_cast<term::ProductType>(term);
            FreeVariables fv;
            for (auto m : product_type->members) {
                fv.InsertWithKeepingStronger(*GetFreeVariables(store, m.type));
            }
            return fv;
        }
    }
}

// Compute and return the actual value of the term.
optional<TermPtr> EvaluateTerm(const EvalContext& ec, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction: {
            auto* fv = GetFreeVariables(ec.store, term);

            BoundVariablesWithParent inner_context(&ec.context);
            auto inner_ec = ec.DuplicateWithDifferentContext(inner_context);

            for (auto [k, v] : fv->variables) {
                // All free variables of this term must be bound.
                assert(ec.context.LookUp(k));
            }
            auto abstraction = term_cast<term::Abstraction>(term);
            for (auto& bv : abstraction->bound_variables) {
                // if bv flows into a type we need to evaluate it fully, otherwise, type only.
                optional<TermPtr> evaluated_bv;
                if (ec.store.DoesVariableFlowsIntoType(bv.variable)) {
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

            auto yet_unbound_implicit_type_parameters =
                unordered_set<term::Variable const*>(BE(abstraction->implicit_type_parameters));
            for (auto& p : abstraction->parameters) {
                bool p_flows_into_type = ec.store.DoesVariableFlowsIntoType(p.variable);
                auto fvs_of_expected_type = GetFreeVariables(ec.store, p.expected_type);
                // The yet unbound implicit variables now bound to an unspecified comptime value.
                for (auto [k, v] : fvs_of_expected_type->variables) {
                    assert(v == FreeVariables::VariableUsage::FlowsIntoType);
                    if (yet_unbound_implicit_type_parameters.count(k) > 0) {
                        inner_context.Bind(k, ec.store.comptime_value);
                        yet_unbound_implicit_type_parameters.erase(k);
                    }
                }
                // Evaluate expected type
                auto evaluated_expected_type =
                    EvaluateTerm(inner_ec.DuplicateWithEvalValues(), p.expected_type);
                if (!evaluated_expected_type) {
                    return nullopt;
                }

                if (inner_ec.eval_values) {
                    if (p_flows_into_type) {
                        // we need the actual value
                    } else {
                        // we need the actual value
                    }
                } else {
                    if (p_flows_into_type) {
                        // we need the actual value
                    } else {
                        // runtime value if fine
                    }
                }
            }

            for (auto& p : abstraction->parameters) {
                if (!p.expected_type) {
                    return nullptr;
                }
                auto evaluated_type =
                    EvaluateTerm(inner_ec.DuplicateWithEvalValues(), *p.expected_type);
                if (!evaluated_type) {
                    return nullptr;
                }
                new_parameters.push_back(term::Parameter{p.variable, *evaluated_type});
            }
            auto evaluated_body = EvaluateTerm(inner_ec, abstraction->body);
            if (!evaluated_body) {
                return nullptr;
            }

            // Might return the same if no change.
            return ec.store.MakeCanonical(term::Abstraction(
                ec.store, move(new_parameters), move(new_bound_variables), *evaluated_body));

        } break;
        case term::Tag::Application: {
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
            /*
            // Evaluate the arguments and compute and return the result.
            auto q =
            vector<TermPtr> evaluated_args;
            for (auto arg : q->arguments) {
                BoundVariablesWithParent temporary_context(ec.context);
                BoundVariablesWithParent inner_context(ec.context);
                auto inner_ec = ec.MakeInner(inner_context);
                if (auto evaluated_arg = EvaluateTerm(inner_ec, arg)) {
                    evaluated_args.push_back(*evaluated_arg);
                } else {
                    return nullopt;
                }
            }
            BoundVariablesWithParent inner_context(ec.context);
            auto inner_ec = ec.MakeInner(inner_context);
            return EvaluateTerm(inner_ec, q->function);
            */
        } break;
        case term::Tag::Variable: {
            // Look up the variable and evaluate it.
            auto variable = term_cast<term::Variable>(term);
            if (auto contents = ec.context.LookUp(variable)) {
                return EvaluateTerm(ec, *contents);
            } else {
                return nullopt;
            }
        } break;
        case term::Tag::Projection: {
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
        case term::Tag::StringLiteral:
        case term::Tag::NumericLiteral:
        case term::Tag::TypeOfTypes:
        case term::Tag::UnitType:
        case term::Tag::BottomType:
        case term::Tag::TopType:
            return term;
        case term::Tag::FunctionType: {
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
        case term::Tag::ProductType: {
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
        case term::Tag::ProductValue: {
            auto q = term_cast<term::ProductValue>(term);
            auto new_type = EvaluateTerm(ec, q->type);
            if (!new_type || (*new_type)->tag != term::Tag::ProductType) {
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
        case term::Tag::StringLiteralType:
        case term::Tag::NumericLiteralType:
            return term;
        case term::Tag::RuntimeValue: {
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
        case term::Tag::UnitLikeValue: {
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

/*
// Return a new term, equivalent to the term parameter, with its type resolved to a concrete,
// normal (irreducible) type, without InferredType terms.
optional<TermPtr> ResolveType(Store& store,
                                    const BoundVariablesWithParent& context,
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
        case term::Tag::Abstraction: {
            auto q = term_cast<term::Abstraction>(term);
            // Types of abstraction can be resolved only if there are no parameters.
            if (!q->parameters.empty()) {
                printf("ResolveTypes: Abstraction with parameters can't be resolved.");
                return nullopt;
            }
            // Resolve types of bound variables
            BoundVariablesWithParent inner_context(context);
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
        case term::Tag::Application: {
            auto q = term_cast<term::Application>(term);
            // TODO eval function first then args then call?
            auto evaluated_function = ComptimeEval(store, context, q->function);
            vector<TermPtr> evaluated_args;
            for (auto arg : q->arguments) {
                evaluated_args.push_back(ComptimeEval(store, context, arg));
            }
        } break;
        case term::Tag::Variable:
            <#code #> break;
        case term::Tag::Projection:
            <#code #> break;
        case term::Tag::StringLiteral:
            <#code #> break;
        case term::Tag::NumericLiteral:
            <#code #> break;
        case term::Tag::TypeOfTypes:
            <#code #> break;
        case term::Tag::UnitType:
            <#code #> break;
        case term::Tag::BottomType:
            <#code #> break;
        case term::Tag::TopType:
            <#code #> break;
        case term::Tag::InferredType:
            <#code #> break;
        case term::Tag::FunctionType:
            <#code #> break;
        case term::Tag::StringLiteralType:
            <#code #> break;
        case term::Tag::NumericLiteralType:
            <#code #> break;
    }
}
*/
/*
// Return resolved pattern (which is usually identical to `concrete`)
optional<TermPtr> UnifyAndInferLeftTypes(Store& store,
                                               const BoundVariablesWithParent& context,
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
                                               BoundVariablesWithParent& context,
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
    BoundVariablesWithParent const* const parent_bound_variables,
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
    BoundVariablesWithParent context(parent_bound_variables);

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
