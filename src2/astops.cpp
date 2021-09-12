#include "astops.h"

#include "module.h"

namespace snl {

term::TermPtr SimplifyAst(term::Store& store, term::TermPtr p)
{
    // TODO: This function does not simplify type terms. It'd be better to remove the LetIn and
    // SequenceYieldLast terms and provide two functions to create those in terms of app/abs.
    using namespace term;
    switch (p->tag) {
        case term::Tag::Abstraction: {
            auto* abstraction = term_cast<Abstraction>(p);
            auto new_body = SimplifyAst(store, abstraction->body);
            if (new_body == abstraction->body) {
                return abstraction;
            }
            return store.MakeCanonical(
                Abstraction(abstraction->type, make_copy(abstraction->parameters), new_body));
        }
        case term::Tag::Application: {
            auto application = term_cast<Application>(p);
            auto new_function = SimplifyAst(store, application->function);
            vector<TermPtr> new_arguments;
            new_arguments.reserve(application->arguments.size());
            bool different = new_function != application->function;
            for (auto a : application->arguments) {
                auto just_pushed = new_arguments.emplace_back(SimplifyAst(store, a));
                different = different || a != just_pushed;
            }
            return different ? store.MakeCanonical(Application(application->type, new_function,
                                                               move(new_arguments)))
                             : application;
        }
        case term::Tag::Variable:
            return p;
        case term::Tag::Projection: {
            auto projection = term_cast<Projection>(p);
            auto new_domain = SimplifyAst(store, projection->domain);
            if (new_domain == projection->domain) {
                return p;
            }
            return store.MakeCanonical(
                Projection(projection->type, new_domain, make_copy(projection->codomain)));
        }
        case term::Tag::StringLiteral:
        case term::Tag::NumericLiteral:
            return p;
        case term::Tag::LetIn: {
            auto let_in = term_cast<LetIn>(p);
            auto abstraction = store.MakeCanonical(Abstraction(
                store.MakeCanonical(FunctionType({let_in->initializer->type, let_in->type})),
                {Parameter{let_in->variable_name}}, SimplifyAst(store, let_in->body)));
            return store.MakeCanonical(
                Application(let_in->type, abstraction, {SimplifyAst(store, let_in->initializer)}));
        }
        case term::Tag::SequenceYieldLast: {
            auto sequence = term_cast<SequenceYieldLast>(p);
            optional<TermPtr> previous_term;
            for (int ix = int(sequence->terms.size()) - 1; ix >= 0; --ix) {
                auto this_term = SimplifyAst(store, sequence->terms[ix]);
                if (previous_term) {
                    previous_term = SimplifyAst(
                        store, store.MakeCanonical(LetIn(make_copy(store.s_ignored_name), this_term,
                                                         *previous_term)));
                } else {
                    previous_term = this_term;
                }
            }
            assert(previous_term.has_value());
            return SimplifyAst(store, previous_term.value_or(store.bottom_type));
        }
        case term::Tag::TypeOfTypes:
        case term::Tag::UnitType:
        case term::Tag::BottomType:
        case term::Tag::TopType:
        case term::Tag::InferredType:
        case term::Tag::FunctionType:
        case term::Tag::StringLiteralType:
        case term::Tag::NumericLiteralType:
            return p;
    }
}

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

struct BoundVariables
{
    unordered_map<string, term::TermPtr> variables;
    unordered_map<int, term::TermPtr> inferred_types;
    optional<term::TermPtr> LookUpInferredType(int id) const
    {
        auto it = inferred_types.find(id);
        return it == inferred_types.end() ? nullopt : make_optional(it->second);
    }
    void append(BoundVariables&& y)
    {
        if (variables.empty()) {
            variables = std::move(y.variables);
        } else {
            variables.insert(BE(y.variables));
        }
        if (inferred_types.empty()) {
            inferred_types = move(y.inferred_types);
        } else {
            inferred_types.insert(BE(y.inferred_types));
        }
    }
};

struct BoundVariablesWithParent : BoundVariables
{
    BoundVariablesWithParent const* const parent = nullptr;
    optional<term::TermPtr> LookUpInferredType(int id) const
    {
        if (auto term = BoundVariables::LookUpInferredType(id)) {
            return term;
        }
        return parent ? parent->LookUpInferredType(id) : nullopt;
    }
};

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
    for (bool allow_inferred_type : {true, false}) {
        switch (pattern->tag) {
            case Tag::Abstraction:
            case Tag::Application:
            case Tag::Variable:
            case Tag::Projection:
            case Tag::StringLiteral:
            case Tag::NumericLiteral:
            case Tag::LetIn:
            case Tag::SequenceYieldLast:
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

optional<term::TermPtr> FixTypes(term::Store& store,
                                 Module& module,
                                 term::TermPtr p,
                                 const vector<term::TermPtr>& args,
                                 BoundVariablesWithParent const* const parent_bound_variables)
{
    using namespace term;
    // Arguments are expected to be resolved types.
    for (auto& arg : args) {
        if (!IsTypeInNormalForm(arg)) {
            printf("Argument is not a type in normal form.\n");
            return nullopt;
        }
    }
    using namespace term;
    switch (p->tag) {
        case term::Tag::Abstraction: {
            auto q = term_cast<Abstraction>(p);
            auto n_applied_args = std::min(args.size(), q->parameters.size());
            BoundVariables bound_variables{parent_bound_variables};
            for (int i = 0; i < n_applied_args; ++i) {
                auto& arg = args[i];
                auto& par = q->parameters[i];
                if (par.type_annotation.has_value()) {
                    UnifyAndInferLeftTypes(*par.type_annotation, arg->type);
                    // Type-annotated parameter.
                    if (par.type_annotation.value() != arg.type) {
                        fmt::print("Different types for parameter {}.", par.name);
                        return nullopt;
                    }
                }
                bound_variables.args[par.name] = arg;
            }
            if (args.size() < lambda_abstraction->parameters.size()) {
                // Return an fn-app which sets the args return that with unknown type
                vector<ast::ExpressionPtr> arguments;
                arguments.resize(n_applied_args);
                new ast::FunctionApplication{lambda_abstraction->body, move(arguments)};
            } else if (args.size() == lambda_abstraction->parameters.size()) {
            } else {
                assert(args.size() > lambda_abstraction->parameters.size());
            }
            //
            //         n_args == n_pars: return resulting expression, the type  expr-type
            //       n_args > n_pars: evaluate expression with the bound vars, apply remaining
            args
                //       to resulting expression
                auto new_body = SimplifyAst(lambda_abstraction->body);
            if (new_body == lambda_abstraction->body) {
                return lambda_abstraction;
            }
            return new ast::LambdaAbstraction{lambda_abstraction->parameters, new_body};
        }
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
        case term::Tag::LetIn:
            <#code #> break;
        case term::Tag::SequenceYieldLast:
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
    /*
        return switch_variant(
            p,
            [&args, parent_bound_variables](ast::LambdaAbstraction* lambda_abstraction) ->
       Result { auto n_applied_args = std::min(args.size(),
       lambda_abstraction->parameters.size()); BoundVariables
       bound_variables{parent_bound_variables}; for (int i = 0; i < n_applied_args; ++i) { auto&
       par = lambda_abstraction->parameters[i]; auto& arg = args[i]; if (par.type_annotation) {
                        // Type-annotated parameter.
                        if (par.type_annotation.value() != arg.type) {
                            fmt::print("Different types for parameter {}.", par.name);
                            return nullopt;
                        }
                    }
                    bound_variables.args[par.name] = arg;
                }
                if (args.size() < lambda_abstraction->parameters.size()) {
                    // Return an fn-app which sets the args return that with unknown type
                    vector<ast::ExpressionPtr> arguments;
                    arguments.resize(n_applied_args);
                    new ast::FunctionApplication{lambda_abstraction->body, move(arguments)};
                } else if (args.size() == lambda_abstraction->parameters.size()) {
                } else {
                    assert(args.size() > lambda_abstraction->parameters.size());
                }
                //
                //         n_args == n_pars: return resulting expression, the type  expr-type
                //       n_args > n_pars: evaluate expression with the bound vars, apply
       remaining args
                //       to resulting expression
                auto new_body = SimplifyAst(lambda_abstraction->body);
                if (new_body == lambda_abstraction->body) {
                    return lambda_abstraction;
                }
                return new ast::LambdaAbstraction{lambda_abstraction->parameters, new_body};
            },
            [](ast::LetExpression* let_expression) -> Result {
                auto lambda_abstraction = new ast::LambdaAbstraction{
                    vector<ast::Parameter>({ast::Parameter{let_expression->variable_name}}),
                    SimplifyAst(let_expression->body)};
                return new ast::FunctionApplication{
                    lambda_abstraction,
                    vector<ast::ExpressionPtr>({SimplifyAst(let_expression->initializer)})};
            },
            [](ast::FunctionApplication* function_application) -> Result {
                auto new_function_expression =
       SimplifyAst(function_application->function_expression); vector<ast::ExpressionPtr>
       new_arguments; new_arguments.reserve(function_application->arguments.size()); bool
       different = new_function_expression != function_application->function_expression; for
       (auto a : function_application->arguments) { auto& just_pushed =
       new_arguments.emplace_back(SimplifyAst(a)); different = different || a != just_pushed;
                }
                if (different) {
                    return new ast::FunctionApplication{new_function_expression,
       move(new_arguments)};
                }
                return function_application;
            },
            [](ast::ExpressionSequence* sequence) -> Result {
                optional<ast::ExpressionPtr> previous_expr;
                for (int ix = int(sequence->expressions.size()) - 1; ix >= 0; --ix) {
                    auto this_expr = SimplifyAst(sequence->expressions[ix]);
                    if (previous_expr) {
                        previous_expr =
                            SimplifyAst(new ast::LetExpression{"_", this_expr, *previous_expr});
                    } else {
                        previous_expr = this_expr;
                    }
                }
                return SimplifyAst(
                    previous_expr.value_or(new ast::BuiltInValue{0}));  // todo return unit.
            },
            [](ast::Variable* p) -> Result { return p; },
            [](ast::NumberLiteral* p) -> Result { return p; },
            [](ast::StringLiteral* p) -> Result { return p; },
            [](ast::Projection* p) -> Result {
                auto new_domain = SimplifyAst(p->domain);
                if (new_domain == p->domain) {
                    return p;
                }
                return new ast::Projection{new_domain, p->codomain};
            },
            [](ast::BuiltInValue* p) -> Result { return p; });
            */
}

}  // namespace snl
