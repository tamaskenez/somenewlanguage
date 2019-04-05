#include "ast.h"

#include "absl/strings/str_format.h"
#include "ul/usual.h"
#include "util/utf.h"

namespace forrest {

using std::in_place_type;

using absl::PrintF;
using absl::StrFormat;

using namespace ul;

Ast::Ast()
{
    storage.emplace_back(in_place_type<TupleNode>);
    _empty_tuplenode = &storage.back();
    storage.emplace_back(in_place_type<VoidLeaf>);
    _voidleaf = &storage.back();
}

void dump(ExprRef er)
{
    struct Visitor
    {
        string ind;
        void indent() { ind += " "; }
        void dedent() { ind.pop_back(); }
        void operator()(const TupleNode& x)
        {
            printf("TUPLE\n");
            indent();
            for (auto& i : x.xs) {
                visit(*this, *i);
            }
            dedent();
        }
        void operator()(const StrNode& x)
        {
            string s;
            for (auto c : x.xs) {
                if (!iscntrl(c) && is_ascii_utf8_byte(c))
                    s += c;
                else {
                    s += StrFormat("\\U+%02X;", c);
                }
            }
            PrintF("%sSTR: \"%s\"\n", ind, s);
        }
        void operator()(const SymLeaf& x)
        {
            PrintF("%sSYM: `%s`\n", ind, (const char*)(x.sym->first.c_str()));
        }
        void operator()(const NumLeaf& x) { PrintF("%sNUM: %s\n", ind, x.x); }
        void operator()(const CharLeaf& x)
        {
            PrintF("%sCHR: %s\n", ind, utf32_to_descriptive_string(x.x));
        }
        void operator()(const VoidLeaf& x) {}
        void operator()(const ApplyNode& x)
        {
            printf("APPLY\n");
            indent();
            (*this)(*x.tuple_ref);
            dedent();
        }
        void operator()(const QuoteNode& x)
        {
            printf("QUOTE\n");
            indent();
            visit(*this, *x.expr_ref);
            dedent();
        }
    };

    visit(Visitor{}, *er);
}

void dump(const Ast& ast)
{
    printf("**** AST DUMP ****\n");
    PrintF("%d top level expressions, %d expressions and %d symbols.\n", ~ast.top_level_exprs,
           ~ast.storage, ~ast.symbols);

    for (auto& e : ast.top_level_exprs) {
        dump(e);
    }
}
}  // namespace forrest
