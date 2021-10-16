#include <cstdlib>

#include "ast.h"
#include "astops.h"
#include "common.h"
#include "samples.h"
#include "store.h"
#include "term.h"

const std::string kCmakeCurrentSourceDir = CMAKE_CURRENT_SOURCE_DIR;

int main(int argc, char* argv[])
{
    using namespace snl;
    Store store;
    auto module = MakeSample1(store);
    auto& tlb = std::get<TopLevelBinding>(module.statements[0]);
    auto main_abstraction = tlb.term;
    Context context(nullptr);
    auto unit_value = store.MakeCanonical(term::UnitLikeValue(store.unit_type));
    auto call_main =
        store.MakeCanonical(term::Application(main_abstraction, vector<TermPtr>({unit_value})));
    auto main_result = EvaluateTerm(store, context, call_main);
    assert(main_result);
    return EXIT_SUCCESS;
}
