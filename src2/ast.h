#pragma once

#include "common.h"
#include "program_type.h"

namespace snl {

namespace ast {

// At this level all identifiers are unique, including all local variables, lambda parameters.
struct Parameter
{
    string name;
    optional<pt::TypePtr> type_annotation;  // Todo: Should be an expression, too.
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
struct UnknownRuntimeValue;

using ExpressionPtr = variant<LambdaAbstraction*,
                              LetExpression*,
                              FunctionApplication*,
                              ExpressionSequence*,
                              Variable*,
                              NumberLiteral*,
                              StringLiteral*,
                              Projection*,
                              BuiltInValue*,
                              UnknownRuntimeValue*>;

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
{};
struct UnknownRuntimeValue
{};
struct UnionValue
{
    pt::TypePtr selector;
    ExpressionPtr value;
};
struct IntersectionValue
{
    unordered_map<pt::TypePtr, ExpressionPtr> values;
};
struct ProductValue
{
    unordered_map<string, ExpressionPtr> values;
};
struct SumValue
{
    string constructor;
    ExpressionPtr value;
};

}  // namespace ast
}  // namespace snl
