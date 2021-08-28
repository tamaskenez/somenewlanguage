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

using ModuleStatement = variant<ToplevelVariableBinding>;

// Context gives a unique key for nodes which share these attributes:
// - same lexical context/scope (lambda abstraction introduces new lexical context)
// - same runtime context/scope (function application introduces a new one)
// One expression (a partical node) might have different types in different runtime contexts.
// Therefore type is not assigned to expression but to context.

struct Context
{
    Context* const parent = nullptr;
};

struct Module
{
    Module(vector<ModuleStatement>&& statements)
        : statements(move(statements)), main_caller_context{&module_context}
    {}
    vector<ModuleStatement> statements;
    Context module_context;
    Context main_caller_context;
    unordered_map<ExpressionPtr, Context*> contextByExpression;
};

Module MakeSample1();

}  // namespace ast

ast::ExpressionPtr SimplifyAst(ast::ExpressionPtr p);
void MarkContexts(ast::Module& module, ast::Context* parent_context, ast::ExpressionPtr p);

}  // namespace snl
