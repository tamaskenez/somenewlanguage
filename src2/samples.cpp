#include "samples.h"
#include "store.h"

namespace snl {
Module MakeSample1(Store& store)
{
    using namespace term;
#define MC store.MakeCanonical
    auto cimport_variable = store.MakeNewVariable("cimport");
    auto stdio_variable = store.MakeNewVariable("stdio");
    auto stdio_initializer =
        MC(Application(cimport_variable, vector<TermPtr>({MC(StringLiteral("#include <stdio>"))})));
    auto stdio_printf = MC(term::Projection(stdio_variable, "printf"));
    auto seq_call_stdio_printf = MC(term::Application(
        stdio_printf, vector<TermPtr>({MC(term::StringLiteral("Print this\n."))})));
    auto main_body_with_stdio = MC(term::Abstraction(
        vector<BoundVariable>(
            {BoundVariable{stdio_variable, stdio_initializer},
             BoundVariable{store.MakeNewVariable(false, make_copy(store.s_ignored_name)),
                           seq_call_stdio_printf}}),
        vector<term::Parameter>(), MC(NumericLiteral("0"))));
    auto main_body = MC(term::Abstraction(
        vector<BoundVariable>({BoundVariable{store.MakeNewVariable("stdio"), stdio_initializer}}),
        vector<Parameter>(), main_body_with_stdio));
    auto main_lambda = MC(term::Abstraction(
        vector<BoundVariable>(),
        vector<Parameter>({Parameter{store.MakeNewVariable(false, make_copy(store.s_ignored_name)),
                                     store.unit_type}}),
        main_body));
    auto cimport_function =
        MC(term::Abstraction(vector<BoundVariable>(),
                             vector<Parameter>({Parameter{store.MakeNewVariable(false, "c_code"),
                                                          store.string_literal_type}}),
                             main_body));
    auto main_def = TopLevelBinding{"main", main_lambda};
    return Module(vector<ModuleStatement>({main_def}));
#undef MC
}
}  // namespace snl
