#include "ast.h"

#include "absl/strings/str_format.h"
#include "ul/usual.h"
#include "util/utf.h"

namespace forrest {

using std::in_place_type;

using absl::PrintF;
using absl::StrFormat;

using namespace ul;

void dump(ExprPtr expr_ptr)
{
    struct Visitor
    {
        string ind;
        string quotes = "";
        void indent() { ind += " "; }
        void dedent() { ind.pop_back(); }
        void tuple(const TupleNode* x)
        {
            PrintF("%s%sTUPLE\n", ind, quotes);
            quotes.clear();
            indent();
            for (auto expr_ptr : x->xs) {
                visit(*this, expr_ptr);
            }
            dedent();
        }
        void apply(ExprPtr lambda, const TupleNode* x)
        {
            PrintF("%s%sAPPLY-TUPLE\n", ind, quotes);
            quotes.clear();
            indent();
            visit(*this, lambda);
            for (auto expr_ptr : x->xs) {
                visit(*this, expr_ptr);
            }
            dedent();
        }
        void operator()(const TupleNode* x) { tuple(x); }
        void operator()(const StrNode* x)
        {
            string s;
            for (auto c : x->xs) {
                if (!iscntrl(c) && is_ascii_utf8_byte(c))
                    s += c;
                else {
                    s += StrFormat("\\U+%02X;", c);
                }
            }
            PrintF("%s%sSTR: \"%s\"\n", ind, quotes, s);
            quotes.clear();
        }
        void operator()(const SymLeaf* x)
        {
            PrintF("%s%sSYM: <%s>\n", ind, quotes, x->name);
            quotes.clear();
        }
        void operator()(const NumLeaf* x)
        {
            PrintF("%s%sNUM: %s\n", ind, quotes, x->x);
            quotes.clear();
        }
        void operator()(const CharLeaf* x)
        {
            PrintF("%s%sCHR: %s\n", ind, quotes, utf32_to_descriptive_string(x->x));
            quotes.clear();
        }
        void operator()(const ApplyNode* x) { apply(x->lambda, x->args); }
        void operator()(const QuoteNode* x)
        {
            quotes += '`';
            visit(*this, x->expr);
        }
    };

    visit(Visitor{}, expr_ptr);
}

}  // namespace forrest
