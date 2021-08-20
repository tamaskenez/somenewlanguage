#pragma once

#include "common.h"

namespace snl {
struct TextAst;

namespace ast {

struct ExpressionWrapper;

struct Unit
{};

using Type = variant<Unit>;

struct StringLiteral
{
    string value;
};

struct Parameter
{
    string name;
    Type type;
};

struct LambdaAbstraction
{
    vector<Parameter> parameters;
    ExpressionWrapper* body;
};

struct LetExpression
{
    string variable_name;
    ExpressionWrapper* initializer;
    ExpressionWrapper* body;
};

struct FunctionApplication
{
    ExpressionWrapper* function_expression;
    vector<ExpressionWrapper*> arguments;
};

struct ExpressionSequence
{
    vector<ExpressionWrapper*> expressions;
};

struct Variable
{
    string value;
};

struct Number
{
    string value;
};

struct Tuple
{
    vector<ExpressionWrapper*> expressions;
};

using Expression = variant<LambdaAbstraction,
                           LetExpression,
                           FunctionApplication,
                           ExpressionSequence,
                           StringLiteral,
                           Variable,
                           Tuple,
                           Number>;

struct ExpressionWrapper
{
    Expression expression;
};

struct ToplevelVariableBinding
{
    string variable_name;
    Expression bound_expression;
};

using ModuleStatement = variant<ToplevelVariableBinding>;
struct Module
{
    vector<ModuleStatement> statements;
};
using ToplevelNode = variant<Module>;
}  // namespace ast

struct Ast
{
    vector<pair<string, ast::ToplevelNode>> toplevel_nodes;
};

optional<Ast> MakeAstFromTextAst(const TextAst& textAst);

}  // namespace snl
