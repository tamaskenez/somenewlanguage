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
    Store store;
    auto module = MakeSample1(store);
    auto& tlb = std::get<TopLevelBinding>(module.statements[0]);
    auto main_abstraction = tlb.term;
    BoundVariablesWithParent context(nullptr);
    auto unit_value = store.MakeCanonical(term::UnitLikeValue(store.unit_type));
    auto call_main =
        store.MakeCanonical(term::Application(store.MakeNewVariable("calling_main_result_type"),
                                              main_abstraction, vector<TermPtr>({unit_value})));
    {
        auto initial_pass_error = InitialPass(context, call_main);
        assert(!initial_pass_error);
    }
    auto ec = EvalContext(store, context,
                          true,  // eval_values, it means we need an executable function, not only
                                 // the type of the function
    );
    auto main_result = EvaluateTerm(ec, call_main);
    assert(main_result);
    return EXIT_SUCCESS;
}
