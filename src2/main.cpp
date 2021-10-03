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
    auto main_abstraction = tlb.term;
    BoundVariablesWithParent context(nullptr);
    auto ec = EvalContext(store, context,
                          true,  // eval_values, it means we need an executable function, not only
                                 // the type of the function
                          false  // don't allow_unbound_variables
    );
    auto evaluated_main_abstraction = EvaluateTerm(ec, main_abstraction);
    vector<term::TermPtr> arguments;
    auto call_main = store.MakeCanonical(term::Application(
        store.MakeNewTypeVariable(), evaluated_main_abstraction.value(), move(arguments)));
    auto main_result = EvaluateTerm(ec, call_main);
    assert(main_result);
    return EXIT_SUCCESS;
}
