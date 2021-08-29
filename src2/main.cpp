#include <cstdlib>

#include "ast.h"
#include "astops.h"
#include "common.h"
#include "samples.h"

const std::string kCmakeCurrentSourceDir = CMAKE_CURRENT_SOURCE_DIR;

int main(int argc, char* argv[])
{
    using namespace snl;
    auto module = MakeSample1();
    auto p = std::get_if<ast::ToplevelVariableBinding>(&module.statements[0]);
    auto simplified = SimplifyAst(p->bound_expression);
    MarkContexts(module, &module.main_caller_context, simplified);
    return EXIT_SUCCESS;
}
