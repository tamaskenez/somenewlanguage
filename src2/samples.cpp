#include "samples.h"

namespace snl {
Module MakeSample1(term::Store& store)
{
    using namespace term;
#define MC store.MakeCanonical
    auto stdio_initializer = MC(Application(
        store.MakeInferredTypeTerm(), MC(Variable(store.MakeInferredTypeTerm(), "cimport")),
        vector<TermPtr>({MC(StringLiteral(store, "#include <stdio>"))})));
    auto stdio_printf =
        MC(term::Projection(store.MakeInferredTypeTerm(),
                            MC(Variable(store.MakeInferredTypeTerm(), "stdio")), "printf"));
    auto main_body_with_stdio = MC(term::SequenceYieldLast(
        store, vector<term::TermPtr>(
                   {MC(term::Application(
                        store.MakeInferredTypeTerm(), stdio_printf,
                        vector<term::TermPtr>({MC(term::StringLiteral(store, "Print this\n."))}))),
                    MC(NumericLiteral(store, "0"))})));
    auto main_body = MC(LetIn("stdio", stdio_initializer, main_body_with_stdio));
    auto main_lambda = MC(term::Abstraction(store.MakeInferredTypeTerm(),
                                            vector<Parameter>({Parameter{"_"}}), main_body));
    auto main_def = TopLevelBinding{"main", main_lambda};
    return Module(vector<ModuleStatement>({main_def}));
#undef MC
}
}  // namespace snl
