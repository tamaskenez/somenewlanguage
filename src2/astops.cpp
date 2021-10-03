#include "astops.h"

#include "module.h"

namespace snl {
/*
ast::ExpressionPtr Compile(Module& module, Context* parent_context, ast::ExpressionPtr e)
{
    switch_variant(
        e,
        [](ast::LambdaAbstraction* lambda_abstraction) {
            return lambda_abstraction;
        },
        [&module, parent_context](ast::FunctionApplication* function_application) {

            assert(false);
                auto new_context = new Context{parent_context};
                module.contextByExpression[function_application->function_expression] = new_context;
                MarkContexts(module, new_context, function_application->function_expression);
                // todo: ask types from args ResolveType(a)
                // then ask result type from function.
                for (auto& a : function_application->arguments) {
                    MarkContexts(module, parent_context, a);
                }
        },
        [&module, parent_context](ast::Projection* p) {
            assert(false);
            // MarkContexts(module, parent_context, p->domain);
        },
        [](ast::Variable* p) {}, [](ast::NumberLiteral* p) { assert(false); },
        [](ast::StringLiteral* p) { assert(false); }, [](ast::BuiltInValue* p) { assert(false); },
        [](ast::LetExpression* let_expression) { assert(false); },
        [](ast::ExpressionSequence* sequence) { assert(false); });
}
*/

struct UnifyResult
{
    term::TermPtr resolved_pattern;
    BoundVariables new_bound_variables;
};

optional<UnifyResult> Unify(term::Store& store,
                            const BoundVariablesWithParent& context,
                            term::TermPtr pattern,
                            term::TermPtr concrete)
{
    assert(false);
    return nullopt;
}

// Compute and return the actual value of the term.
optional<term::TermPtr> EvaluateTerm(const EvalContext& ec, term::TermPtr term)
{
    switch (term->tag) {
        case term::Tag::Abstraction: {
            if (!ec.eval_values) {
                if (auto type_of_term =
                        EvaluateTerm(EvalContext(ec.store, ec.context, true, false), term->type)) {
                    return ec.store.MakeCanonical(term::RuntimeValue(*type_of_term));
                }
            }
            // At this point we either need values or weren't able to resolve the type just by
            // looking at the parameters.
            auto abstraction = term::term_cast<term::Abstraction>(term);
            if (abstraction->parameters.empty()) {
                // No parameters: compute and return the result.
                BoundVariablesWithParent inner_context(ec.context);
                auto inner_ec = ec.MakeInner(inner_context);
                for (auto& bv : abstraction->bound_variables) {
                    if (auto evaluated_bv = EvaluateTerm(inner_ec, bv.value)) {
                        // evaluated_bv is the value of the bound variable.
                        // evaluated_bv has a concrete type and might have a compile-time value.
                        // bv has a type, at least a type variable.
                        // Unify the type of the content with the the expectation, bv's type.
                        if (auto ur = Unify(ec.store, inner_context, bv.variable->type,
                                            (*evaluated_bv)->type)) {
                            // Add new bound variables to inner_context, even subsequent bound
                            // variables might use it.
                            inner_context.append(move(ur->new_bound_variables));
                            // Add the bound variable's value.
                            // Now we have
                            // - the resolved pattern
                            // - the concrete type of the value
                            // - we might have the value, too.
                            // I don't know what to do if the types are different, we'll see it
                            // later.
                            assert(ur->resolved_pattern == bv.variable->type);
                            inner_context.Bind(bv.variable, *evaluated_bv);
                        } else {
                            // bound variable expected type can't be unified with the actual value.
                            return nullopt;
                        }
                    } else {
                        printf("Can't evaluate abstractions's bound variable.");
                        return nullopt;
                    }
                }
                return EvaluateTerm(EvalContext{ec.store, inner_context, ec.eval_values,
                                                ec.allow_unbound_variables},
                                    abstraction->body);
            } else {
                // If it has parameters:
                // If all parameters have concrete types then we resolve the entire abstractions
                // with concrete types. What does eva_values mean in this case? If some parameters
                // have no concrete type, what to do?
                // eval_values allow_unbound all-parameters-have-types
                // 0 0 0 nullopt
                // 0 0 1 bind unbound variables to types with runtime values and resolve return type
                // 0 1 0 bind unbound variables to types with runtime values and resolve return
                // type, don't mind unbound 0 1 1 bind unbound variables to types with runtime
                // values and resolve return type, don't mind unbound 1 0 0 nullopt 1 0 1 bind
                // unbound variables to types with runtime values and resolve whole thing 1 1 0 bind
                // unbound variables to types with runtime values and resolve return type, don't
                // mind unbound 1 1 1 bind unbound variables to types with runtime values and
                // resolve return type, don't mind unbound
                BoundVariablesWithParent inner_context(ec.context);
                for (auto& p : abstraction->parameters) {
                    if (auto et = EvaluateTerm(
                            EvalContext{ec.store, inner_context, true, ec.allow_unbound_variables},
                            p.variable->type)) {
                        // We have the type of this parameter.
                        // A variable always has a concrete, expected type
                        // It might be bound to unknown runtime value with the expected type
                        // or to a concrete value which might be used
                        // or to a concrete value which must be used
                        // variable               value

                        // runtime value          runtime value : use the expected type
                        // runtime value          comptime value: ignore, use the expected type
                        // optional comptime      runtime value: use the expected type

                        // optional comptime      comptime value: specialize on the value
                        // oblib comptime         comptime value: specialize on the value

                        // oblig comptime         runtime value: failure
                    } else {
                        // Probably this parameter's type is a variable waiting to be unified with
                        // an actual argument. Don't know what to do here.
                        assert(false);
                    }
                }
                assert(false);
            }
        } break;
        case term::Tag::Application: {
            // Evaluate the arguments and compute and return the result.
            auto q = term::term_cast<term::Application>(term);
            vector<term::TermPtr> evaluated_args;
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
        } break;
        case term::Tag::Variable: {
            // Look up the variable and evaluate it.
            auto variable = term::term_cast<term::Variable>(term);
            if (auto contents = ec.context.LookUp(variable)) {
                return EvaluateTerm(ec, *contents);
            } else {
                return nullopt;
            }
        } break;
        case term::Tag::Projection: {
            // Perform the projection and evaluate it.
            auto q = term::term_cast<term::Projection>(term);
            if (auto evaluated_domain_ = EvaluateTerm(ec, q->domain)) {
                auto evaluated_domain = *evaluated_domain_;
                if (evaluated_domain->tag == term::Tag::ProductValue) {
                    auto product_value = term::term_cast<term::ProductValue>(evaluated_domain);
                    auto product_type = term::term_cast<term::ProductType>(product_value->type);
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
            auto q = term::term_cast<term::FunctionType>(term);
            vector<term::TermPtr> resolved_terms;
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
            auto q = term::term_cast<term::ProductType>(term);
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
            auto q = term::term_cast<term::ProductValue>(term);
            auto new_type = EvaluateTerm(ec, q->type);
            if (!new_type || (*new_type)->tag != term::Tag::ProductType) {
                return nullopt;
            }
            vector<term::TermPtr> new_values;
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
            return ec.store.MakeCanonical(term::ProductValue(
                term::term_cast<term::ProductType>(*new_type), move(new_values)));
        } break;
        case term::Tag::StringLiteralType:
        case term::Tag::NumericLiteralType:
            return term;
        case term::Tag::RuntimeValue: {
            auto runtime_value = term::term_cast<term::RuntimeValue>(term);
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
    }
    assert(false);
    return nullopt;
}

/*
// Return a new term, equivalent to the term parameter, with its type resolved to a concrete,
// normal (irreducible) type, without InferredType terms.
optional<term::TermPtr> ResolveType(term::Store& store,
                                    const BoundVariablesWithParent& context,
                                    term::TermPtr term)
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
            auto q = term::term_cast<term::Abstraction>(term);
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
            auto q = term::term_cast<term::Application>(term);
            // TODO eval function first then args then call?
            auto evaluated_function = ComptimeEval(store, context, q->function);
            vector<term::TermPtr> evaluated_args;
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
optional<term::TermPtr> UnifyAndInferLeftTypes(term::Store& store,
                                               const BoundVariablesWithParent& context,
                                               term::TermPtr pattern,
                                               term::TermPtr concrete,
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

optional<term::TermPtr> UnifyAndInferLeftTypes(term::Store& store,
                                               BoundVariablesWithParent& context,
                                               term::TermPtr pattern,
                                               term::TermPtr concrete)
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

std::optional<term::TermPtr> ApplyArgumentsToAbstraction(
    term::Store& store,
    BoundVariablesWithParent const* const parent_bound_variables,
    term::Abstraction const* abstraction,
    const vector<term::TermPtr>& args)
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
    auto new_arguments = vector<term::TermPtr>(args.begin() + n_args_apply, args.end());
    return store.MakeCanonical(
        term::Application(new_abstraction->ResultType(), new_abstraction, move(new_arguments)));
}
*/

}  // namespace snl
