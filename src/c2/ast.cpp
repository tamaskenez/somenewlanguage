#include "ast.h"

#include "absl/strings/str_format.h"
#include "ul/usual.h"

#include "ast_syntax.h"
#include "util/utf.h"

namespace forrest {

using absl::PrintF;
using absl::StrFormat;
using namespace ul;

void dump(ast::Expr* expr)
{
    struct Visitor
    {
        string ind;
        void indent() { ind += " "; }
        void dedent() { ind.pop_back(); }
        void operator()(const ast::List& x)
        {
            PrintF("%s(\n", ind);
            indent();
            for (auto expr_ptr : x.xs) {
                visit(*this, *expr_ptr);
            }
            dedent();
            PrintF("%s)\n", ind);
        }
        void operator()(const ast::Token& x)
        {
            PrintF("%s", ind);
            switch (x.kind) {
                case ast::Token::STRING:
                    break;
                case ast::Token::QUOTED_STRING:
                    break;
                    PrintF("<QUOTED>: ");
                case ast::Token::NUMBER:
                    break;
                    PrintF("<NUMBER>: ");
                default:
                    UL_UNREACHABLE;
            }
            string s;
            for (auto c : x.x) {
                if (is_ascii_utf8_byte(c) && (c == ' ' || isgraph(c))) {
                    s += c;
                } else {
                    s += StrFormat("\\U+%02X;", c);
                }
            }
            PrintF("%s\n", s);
        }
    };

    visit(Visitor{}, *expr);
}

bool is_env_args_separator(ast::Expr* e)
{
    auto t = get_if<ast::Token>(e);
    return t && t->x == ENV_ARGS_SEPARATOR;
}
}  // namespace forrest
