#include "ast.h"

#include <any>
#include <typeinfo>

namespace snl {

namespace ast {

Module MakeSample1()
{
    auto stdio_initializer = new FunctionApplication{
        new Variable{"cimport"}, vector<ExpressionPtr>({new StringLiteral{"#include <stdio>"}})};
    auto stdio_printf = new Projection{new Variable{"stdio"}, "printf"};
    auto main_body_with_stdio = new ExpressionSequence{vector<ExpressionPtr>(
        {new FunctionApplication{stdio_printf,
                                 vector<ExpressionPtr>({new StringLiteral{"Print this\n."}})},
         new NumberLiteral{"0"}})};
    auto main_body = new LetExpression{"stdio", stdio_initializer, main_body_with_stdio};
    auto main_lambda = new LambdaAbstraction{vector<Parameter>(), main_body};
    auto main_def = ToplevelVariableBinding{"main", main_lambda};
    return Module(vector<ModuleStatement>({main_def}));
}

}  // namespace ast

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

void MarkContexts(ast::Module& module, ast::Context* parent_context, ast::ExpressionPtr e)
{
    switch_variant(
        e,
        [&module, parent_context](ast::LambdaAbstraction* lambda_abstraction) {
            auto new_context = new ast::Context{parent_context};
            module.contextByExpression[lambda_abstraction->body] = new_context;
            MarkContexts(module, new_context, lambda_abstraction->body);
        },
        [&module, parent_context](ast::FunctionApplication* function_application) {
            auto new_context = new ast::Context{parent_context};
            module.contextByExpression[function_application->function_expression] = new_context;
            MarkContexts(module, new_context, function_application->function_expression);
            for (auto& a : function_application->arguments) {
                MarkContexts(module, parent_context, a);
            }
        },
        [&module, parent_context](ast::Projection* p) {
            MarkContexts(module, parent_context, p->domain);
        },
        [](ast::Variable* p) {}, [](ast::NumberLiteral* p) {}, [](ast::StringLiteral* p) {},
        [](ast::BuiltInValue* p) {}, [](ast::LetExpression* let_expression) { assert(false); },
        [](ast::ExpressionSequence* sequence) { assert(false); });
}
}  // namespace snl
