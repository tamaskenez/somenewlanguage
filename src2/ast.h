#pragma once

#include "common.h"

namespace snl {
struct TextAst;

struct ProgramType
{};

namespace ast {

struct Parameter
{
    string name;
    ProgramType* type_annotation = nullptr;
};

struct LambdaAbstraction;
struct LetExpression;
struct FunctionApplication;
struct ExpressionSequence;
struct Variable;
struct NumberLiteral;
struct StringLiteral;
struct Projection;
struct BuiltInValue;

using ExpressionPtr = variant<LambdaAbstraction*,
                              LetExpression*,
                              FunctionApplication*,
                              ExpressionSequence*,
                              Variable*,
                              NumberLiteral*,
                              StringLiteral*,
                              Projection*,
                              BuiltInValue*>;

struct LambdaAbstraction
{
    vector<Parameter> const parameters;
    ExpressionPtr const body;
};

struct LetExpression
{
    string const variable_name;
    ExpressionPtr const initializer;
    ExpressionPtr const body;
};

struct FunctionApplication
{
    ExpressionPtr const function_expression;
    vector<ExpressionPtr> const arguments;
};

struct ExpressionSequence
{
    vector<ExpressionPtr> const expressions;
};

struct Variable
{
    string const value;
};

struct NumberLiteral
{
    string const value;
};

struct StringLiteral
{
    string const value;
};

struct BuiltInValue
{
    int const index = 0;
};

struct Tuple
{
    vector<ExpressionPtr> const expressions;
};

struct Projection
{
    ExpressionPtr const domain;
    string const codomain;
};

struct ToplevelVariableBinding
{
    string const variable_name;
    ExpressionPtr const bound_expression;
};

using ModuleStatement = variant<ToplevelVariableBinding>;
struct Module
{
    vector<ModuleStatement> statements;
};

Module MakeSample1();

}  // namespace ast

ast::ExpressionPtr SimplifyAst(ast::ExpressionPtr p);

}  // namespace snl
