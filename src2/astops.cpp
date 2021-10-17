
namespace snl {

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
