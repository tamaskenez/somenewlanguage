#pragma once

#include "common.h"
#include "program_type.h"

namespace snl {

namespace ast {

struct Parameter
{
    string name;
    pt::TypePtr type_annotation = nullptr;
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
struct UnaryLambdaAbstraction;
struct UnaryFunctionApplication;

using ExpressionPtr = variant<LambdaAbstraction*,
                              LetExpression*,
                              FunctionApplication*,
                              ExpressionSequence*,
                              Variable*,
                              NumberLiteral*,
                              StringLiteral*,
                              Projection*,
                              BuiltInValue*>;

using UExpressionPtr = variant<UnaryLambdaAbstraction*,
                               UnaryFunctionApplication*,
                               Variable*,
                               NumberLiteral*,
                               StringLiteral*,
                               Projection*,
                               BuiltInValue*>;

// These will be simplified away.
struct LetExpression
{
    string const variable_name;
    ExpressionPtr const initializer;
    ExpressionPtr const body;
};
struct ExpressionSequence
{
    vector<ExpressionPtr> const expressions;
};

struct LambdaAbstraction
{
    vector<Parameter> const parameters;
    ExpressionPtr const body;
};

struct FunctionApplication
{
    ExpressionPtr const function_expression;
    vector<ExpressionPtr> const arguments;
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

// Unary expressions.
struct ULambdaAbstraction
{
    Parameter const parameter;
    ExpressionPtr const body;
};

struct UFunctionApplication
{
    ExpressionPtr const function_expression;
    ExpressionPtr const argument;
};

struct UTuple
{
    vector<UExpressionPtr> const expressions;
};

struct UProjection
{
    UExpressionPtr const domain;
    string const codomain;
};

struct ToplevelVariableBinding
{
    string const variable_name;
    ExpressionPtr const bound_expression;
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

}  // namespace ast
}  // namespace snl
