#pragma once

#include "ast.h"
#include "program_type.h"
#include "term.h"

namespace snl {
struct Module;
struct Context;
term::TermPtr SimplifyAst(term::Store& store, term::TermPtr p);
void MakeCompiledFunction(Module& module,
                          Context* parent_context,
                          ast::ExpressionPtr p,
                          const vector<pt::TypePtr>& parameter_types);
}  // namespace snl
