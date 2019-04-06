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
        string quotes = "";
        void indent() { ind += " "; }
        void dedent() { ind.pop_back(); }
        void apply_or_tupple(const TupleNode& x, bool apply)
        {
            PrintF("%s%s%s\n", ind, quotes, apply ? "APPLY-TUPLE" : "TUPLE");
            quotes.clear();
            indent();
            for (auto& i : x.xs) {
                visit(*this, *i);
            }
            dedent();
        }
        void operator()(const TupleNode& x) { apply_or_tupple(x, false); }
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
            PrintF("%s%sSTR: \"%s\"\n", ind, quotes, s);
            quotes.clear();
        }
        void operator()(const SymLeaf& x)
        {
            PrintF("%s%sSYM: <%s>\n", ind, quotes, (const char*)(x.sym->first.c_str()));
            quotes.clear();
        }
        void operator()(const NumLeaf& x)
        {
            PrintF("%s%sNUM: %s\n", ind, quotes, x.x);
            quotes.clear();
        }
        void operator()(const CharLeaf& x)
        {
            PrintF("%s%sCHR: %s\n", ind, quotes, utf32_to_descriptive_string(x.x));
            quotes.clear();
        }
        void operator()(const VoidLeaf& x) {}
        void operator()(const ApplyNode& x) { apply_or_tupple(*x.tuple_ref, true); }
        void operator()(const QuoteNode& x)
        {
            quotes += '`';
            visit(*this, *x.expr_ref);
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
