#pragma once

#include "ast.h"
#include "program_type.h"

namespace snl {
struct Module;
struct Context;
ast::ExpressionPtr SimplifyAst(ast::ExpressionPtr p);
void MarkContexts(Module& module, Context* parent_context, ast::ExpressionPtr p, pt::TypePtr type);
}  // namespace snl
