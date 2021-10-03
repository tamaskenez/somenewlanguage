#include "samples.h"

namespace snl {
Module MakeSample1(term::Store& store)
{
    using namespace term;
#define MC store.MakeCanonical
    auto cimport_variable = store.MakeNewVariable("cimport");
    auto stdio_initializer =
        MC(Application(store.MakeNewTypeVariable(), cimport_variable,
                       vector<TermPtr>({MC(StringLiteral(store, "#include <stdio>"))})));
    auto stdio_printf = MC(term::Projection(
        store.MakeNewTypeVariable(), new Variable(store.MakeNewTypeVariable(), "stdio"), "printf"));
    auto seq_item1 = MC(term::Application(
        store.MakeNewTypeVariable(), stdio_printf,
        vector<term::TermPtr>({MC(term::StringLiteral(store, "Print this\n."))})));
    auto seq_item2 = MC(NumericLiteral(store, "0"));
    auto main_body_with_stdio = MC(term::Abstraction(
        store, vector<Parameter>(),
        vector<BoundVariable>(
            {BoundVariable{store.MakeNewVariable(make_copy(store.s_ignored_name)), seq_item1}}),
        seq_item2));
    auto main_body = MC(term::Abstraction(
        store, vector<Parameter>(),
        vector<BoundVariable>({BoundVariable{store.MakeNewVariable("stdio"), stdio_initializer}}),
        main_body_with_stdio));
    auto main_lambda = MC(term::Abstraction(
        store,
        vector<Parameter>({Parameter{store.MakeNewVariable(make_copy(store.s_ignored_name))}}),
        vector<BoundVariable>(), main_body));
    auto main_def = TopLevelBinding{"main", main_lambda};
    return Module(vector<ModuleStatement>({main_def}));
#undef MC
}
}  // namespace snl
