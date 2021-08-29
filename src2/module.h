#pragma once

#include "ast.h"
#include "common.h"

namespace snl {

using ModuleStatement = variant<ast::ToplevelVariableBinding>;

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
    unordered_map<ast::ExpressionPtr, Context*> contextByExpression;
};
}  // namespace snl
