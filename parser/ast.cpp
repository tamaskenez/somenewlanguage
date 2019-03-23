#include "ast.h"

#include "absl/strings/str_format.h"
#include "ul/usual.h"

namespace forrest {

using absl::PrintF;
using namespace ul;

void dump(const Ast& ast)
{
    printf("**** AST DUMP ****\n");
    PrintF("%d top level expressions, %d expressions and %d symbols.\n", ~ast.top_level_exprs,
           ~ast.storage, ~ast.symbols);

    struct Visitor
    {
        string ind;
        void indent() { ind += " "; }
        void dedent() { ind.pop_back(); }
        void operator()(const VecNode& x)
        {
            PrintF("%s%s:\n", ind, (x.apply ? "APP" : "VEC"));
            indent();
            for (auto& i : x.xs) {
                visit(*this, *i);
            }
            dedent();
        }
        void operator()(const StrNode& x)
        {
            PrintF("%sSTR: \"%s\"\n", ind, (const char*)(x.xs.c_str()));
        }
        void operator()(const SymLeaf& x)
        {
            PrintF("%sSYM: `%s`\n", ind, (const char*)(x.sym->first.c_str()));
        }
        void operator()(const NumLeaf& x) { PrintF("%sNUM: %s\n", ind, x.x); }
        void operator()(const CharLeaf& x) { PrintF("%sCHR: %d\n", ind, x.x); }
    };

    for (auto& e : ast.top_level_exprs) {
        visit(Visitor{}, *e);
    }
}
}  // namespace forrest
