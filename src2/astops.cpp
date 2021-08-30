#include "astops.h"
#include "module.h"

namespace snl {
ast::ExpressionPtr SimplifyAst(ast::ExpressionPtr p)
{
    return switch_variant(
        p,
        [](ast::LambdaAbstraction* lambda_abstraction) -> ast::ExpressionPtr {
            auto new_body = SimplifyAst(lambda_abstraction->body);
            if (new_body == lambda_abstraction->body) {
                return lambda_abstraction;
            }
            return new ast::LambdaAbstraction{lambda_abstraction->parameters, new_body};
        },
        [](ast::LetExpression* let_expression) -> ast::ExpressionPtr {
            auto lambda_abstraction = new ast::LambdaAbstraction{
                vector<ast::Parameter>({ast::Parameter{let_expression->variable_name}}),
                SimplifyAst(let_expression->body)};
            return new ast::FunctionApplication{
                lambda_abstraction,
                vector<ast::ExpressionPtr>({SimplifyAst(let_expression->initializer)})};
        },
        [](ast::FunctionApplication* function_application) -> ast::ExpressionPtr {
            auto new_function_expression = SimplifyAst(function_application->function_expression);
            vector<ast::ExpressionPtr> new_arguments;
            new_arguments.reserve(function_application->arguments.size());
            bool different = new_function_expression != function_application->function_expression;
            for (auto a : function_application->arguments) {
                auto& just_pushed = new_arguments.emplace_back(SimplifyAst(a));
                different = different || a != just_pushed;
            }
            if (different) {
                return new ast::FunctionApplication{new_function_expression, move(new_arguments)};
            }
            return function_application;
        },
        [](ast::ExpressionSequence* sequence) -> ast::ExpressionPtr {
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
        [](ast::Variable* p) -> ast::ExpressionPtr { return p; },
        [](ast::NumberLiteral* p) -> ast::ExpressionPtr { return p; },
        [](ast::StringLiteral* p) -> ast::ExpressionPtr { return p; },
        [](ast::Projection* p) -> ast::ExpressionPtr {
            auto new_domain = SimplifyAst(p->domain);
            if (new_domain == p->domain) {
                return p;
            }
            return new ast::Projection{new_domain, p->codomain};
        },
        [](ast::BuiltInValue* p) -> ast::ExpressionPtr { return p; });
}

void MarkContexts(Module& module, Context* parent_context, ast::ExpressionPtr e, pt::TypePtr type)
{
    switch_variant(
        e,
        [&module, parent_context](ast::LambdaAbstraction* lambda_abstraction) {
            // Parameters must have compatible type. Create appropriate variables in context with
            // intersection types.

            lambda_abstraction->parameters;
            lambda_abstraction->body;
            assert(false);
            /*
                auto new_context = new Context{parent_context};
                module.contextByExpression[lambda_abstraction->body] = new_context;
                MarkContexts(module, new_context, lambda_abstraction->body);
                */
        },
        [&module, parent_context](ast::FunctionApplication* function_application) {
            assert(false);
            /*
                auto new_context = new Context{parent_context};
                module.contextByExpression[function_application->function_expression] = new_context;
                MarkContexts(module, new_context, function_application->function_expression);
                // todo: ask types from args ResolveType(a)
                // then ask result type from function.
                for (auto& a : function_application->arguments) {
                    MarkContexts(module, parent_context, a);
                }
                */
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
}  // namespace snl
