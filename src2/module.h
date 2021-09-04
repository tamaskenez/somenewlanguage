#pragma once

#include "common.h"
#include "term.h"

namespace snl {

using ModuleStatement = variant<term::ToplevelVariableBinding>;

// Context gives a unique key for nodes which share these attributes:
// - same lexical context/scope (lambda abstraction introduces new lexical context)
// - same runtime context/scope (function application introduces a new one)
// One expression (a partical node) might have different types in different runtime contexts.
// Therefore type is not assigned to expression but to context.

struct Module
{
    Module(vector<ModuleStatement>&& statements) : statements(move(statements)) {}
    vector<ModuleStatement> statements;
};
}  // namespace snl
