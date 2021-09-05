#include <cstdlib>

#include "ast.h"
#include "astops.h"
#include "common.h"
#include "samples.h"
#include "term.h"

const std::string kCmakeCurrentSourceDir = CMAKE_CURRENT_SOURCE_DIR;

int main(int argc, char* argv[])
{
    using namespace snl;
    term::Store store;
    auto module = MakeSample1(store);
    auto& tlb = std::get<TopLevelBinding>(module.statements[0]);
    auto simplified = SimplifyAst(store, tlb.term);
    auto unit_to_unit = store.MakeCanonical(term::FunctionType({store.unit_type, store.unit_type}));
    MakeCompiledFunction(store, module, simplified, {store.unit_type});
    return EXIT_SUCCESS;
}
