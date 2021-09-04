#include "samples.h"

namespace snl {
Module MakeSample1(term::Store& store)
{
    using namespace term;
#define MC store.MakeCanonical
    auto stdio_initializer = MC(Application(
        store.GetNextTypeVariable(), MC(Variable(store.GetNextTypeVariable(), "cimport")),
        vector<TermPtr>({MC(StringLiteral(store, "#include <stdio>"))})));
    /*
auto stdio_printf = new Projection{new Variable{"stdio"}, "printf"};
auto main_body_with_stdio = new ExpressionSequence{vector<ExpressionPtr>(
    {new FunctionApplication{stdio_printf,
                             vector<ExpressionPtr>({new StringLiteral{"Print this\n."}})},
     new NumberLiteral{"0"}})};
auto main_body = new LetExpression{"stdio", stdio_initializer, main_body_with_stdio};
auto main_lambda = new LambdaAbstraction{vector<Parameter>(), main_body};
auto main_def = ToplevelVariableBinding{"main", main_lambda};
return Module(vector<ModuleStatement>({main_def}));
*/
#undef MC
}
}  // namespace snl
