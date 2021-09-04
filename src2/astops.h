#pragma once

#include "ast.h"
#include "program_type.h"

namespace snl {
struct Module;
struct Context;
ast::ExpressionPtr SimplifyAst(ast::ExpressionPtr p);
    void MakeCompiledFunction(Module& module, Context* parent_context, ast::ExpressionPtr p, const vector<pt::TypePtr> &parameter_types);
}  // namespace snl
