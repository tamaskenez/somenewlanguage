#include "ast_builder.h"

#include <unordered_map>

#include "absl/strings/str_format.h"
#include "ul/usual.h"
#include "util/arena.h"
#include "util/filereader.h"

#include "ast.h"
#include "ast_syntax.h"
#include "command_line.h"
#include "errors.h"

namespace forrest {

using std::get;
using std::holds_alternative;
using std::in_place_type;
using std::move;

using namespace ul;

using absl::StrFormat;

class AstBuilderImpl
{
    FileReader& fr;
    Ast& ast;

    string error;

    struct CharLC
    {
        char c;
        int line, col;
    };

public:
    AstBuilderImpl(FileReader& fr, Ast& ast) : fr(fr), ast(ast) {}
    either<string, vector<ast::Expr*>> run()
    {
        vector<ast::Expr*> top_level_exprs;
        for (;;) {
            skip_whitespace_and_comments();
            if (fr.n_unread_chars() == 0) {
                if (auto me = fr.maybe_get_error()) {
                    return *me;
                }
                return top_level_exprs;
            }
            auto mr = read_expr();
            if (mr) {
                top_level_exprs.emplace_back(*mr);
            } else {
                return error;
            }
        }
    }

private:
    void skip_whitespace_and_comments()
    {
        for (;;) {
            fr.skip_whitespace();
            if (fr.n_unread_chars() == 0) {
                return;
            }
            if (fr.attempt_wora(COMMENT_CHAR)) {
                // Skip until end of line
                int line = fr.line();
                for (;;) {
                    auto c = fr.next_char();
                    if (!c || fr.line() != line) {
                        break;
                    }
                }
            } else {
                return;
            }
        }
    }
    // Whitespace skipped before this.
    maybe<ast::Expr*> read_expr()
    {
        if (!fr.read_ahead_at_least_1()) {
            report_error();
            return {};
        }
        if (fr.attempt_wora(OPEN_LIST_CHAR)) {  // Start Exprs
            return read_list();
        } else if (fr.attempt_wora(STRING_QUOTE_CHAR)) {  // Start AsciiStr/Str
            return read_str();
        } else if (fr.peek_wora(
                       [](Utf8Char c) { return c == '-' || c == '+' || isdigit(c.front()); })) {
            return read_num();
        } else {
            return read_sym();
        }

        /* else if (fr.attempt_wora(OPEN_APPLY_CHAR)) {  // Start function application.
            return read_apply();
        }  else if (fr.attempt_wora(UTFCHAR_PREFIX)) {  // Start utfchar
            auto mc = read_utf8char();
            if (!mc) {
                if (error.empty()) {
                    report_error();
                }
                return {};
            }
            auto mcp = mc->code_point();
            if (!mcp) {
                // This actually cannot happen since FileReader doesn't let through invalid
                // UTF8.
                report_error("Invalid UTF8 character literal at");
                return {};
            }
            return storage.new_<CharLeaf>(*mcp);
        } else if (fr.attempt_wora(QUOTE_CHAR)) {
            return read_quote();
        }
        */
        UL_UNREACHABLE;
    }

    /*
    maybe<ApplyNode*> read_apply()
    {
        auto mx = read_tuple_to_vector(OPEN_APPLY_CHAR, CLOSE_APPLY_CHAR);
        if (!mx)
            return {};
        auto& xs = *mx;
        if (xs.empty()) {
            fprintf(stderr, "Empty {}.\n");
            return {};
        }
        auto tn = storage.new_<TupleNode>(xs.begin() + 1, xs.end());
        return storage.new_<ApplyNode>(xs.front(), tn);
    };

    maybe<QuoteNode*> read_quote()
    {
        maybe<Node*> mx = read_expr();
        if (!mx)
            return {};
        return storage.new_<QuoteNode>(*mx);
    }
*/
    maybe<ast::Expr*> read_list()
    {
        auto mt = read_list_to_vector();
        if (!mt)
            return {};
        return ast.new_list(move(*mt));
    }

    maybe<vector<ast::Expr*>> read_list_to_vector()
    {
        CharLC open_char_lc{OPEN_LIST_CHAR, fr.line(), fr.col()};
        vector<ast::Expr*> xs;
        for (;;) {
            skip_whitespace_and_comments();
            if (!fr.read_ahead_at_least_1()) {
                report_missing_char(CLOSE_LIST_CHAR, open_char_lc);
                return {};
            }
            if (fr.attempt_wora(CLOSE_LIST_CHAR)) {
                return xs;
            };
            auto mx = read_expr();
            if (!mx)
                return {};
            // An item must be followed by whitespace or closing char or comment.
            if (!fr.peek_wora([](Utf8Char c) {
                    return c == CLOSE_LIST_CHAR || c == COMMENT_CHAR || isspace(c.front());
                })) {
                report_error();
                return {};
            }
            xs.emplace_back(*mx);
        }
        UL_UNREACHABLE;
    }
    maybe<Utf8Char> read_utf8char()
    {
        auto m_nc = fr.next_char();
        if (!m_nc)
            return {};

        auto nc = *m_nc;
        if (nc == STRING_ESCAPE_CHAR) {
            m_nc = fr.next_char();
            if (!m_nc) {
                report_error();
                return {};
            }
            nc = *m_nc;
            switch (nc.front()) {
                case '0':
                    nc = '\0';
                    break;
                case 't':
                    nc = '\t';
                    break;
                case 'n':
                    nc = '\n';
                    break;
                case 'r':
                    nc = '\r';
                    break;
                case '\\':
                    nc = '\\';
                    break;
                case '\"':
                    nc = '\"';
                    break;
                default:
                    report_error(StrFormat("Invalid escape char %s at", to_descriptive_string(nc)));
                    return {};
            }
        }
        return nc;
    }

    maybe<ast::Expr*> read_str()
    {
        CharLC begin_char{STRING_QUOTE_CHAR, fr.line(), fr.col()};
        string xs;
        for (;;) {
            auto m_nc = read_utf8char();
            if (!m_nc) {
                if (error.empty()) {
                    report_missing_char(STRING_QUOTE_CHAR, begin_char);
                }
                return {};
            }
            if (*m_nc == STRING_QUOTE_CHAR) {
                return ast.new_token(move(xs), ast::Token::QUOTED_STRING);
            }
            xs.append(BE(*m_nc));
        }
    }

    maybe<ast::Expr*> read_num()
    {
        enum State
        {
            NEED_SIGN_OR_GO_ON,
            NEED_SINGLE_ZERO_OR_NONZERO_INTEGER,
            FINISHING_NONZERO_INTEGER,
            MAYBE_DECIMAL_DOT,
            AFTER_DECIMAL_DOT,
            DONE
        } state = NEED_SIGN_OR_GO_ON;

        string xs;
        do {
            auto m_nc = fr.peek_char();
            switch (state) {
                case NEED_SIGN_OR_GO_ON:
                    if (m_nc) {
                        if (*m_nc == '+') {
                            fr.next_char();
                        } else if (*m_nc == '-') {
                            fr.next_char();
                            xs += '-';
                        }
                        state = NEED_SINGLE_ZERO_OR_NONZERO_INTEGER;
                    } else {
                        report_error();
                        return {};
                    }
                    break;
                case NEED_SINGLE_ZERO_OR_NONZERO_INTEGER:
                    if (m_nc) {
                        if (*m_nc == '0') {
                            fr.next_char();
                            xs += '0';
                            state = MAYBE_DECIMAL_DOT;
                            break;
                        } else if (isdigit(m_nc->front())) {
                            fr.next_char();
                            xs += m_nc->front();
                            state = FINISHING_NONZERO_INTEGER;
                            break;
                        }
                    }
                    report_error();
                    return {};
                case FINISHING_NONZERO_INTEGER:
                    if (m_nc) {
                        if (isdigit(m_nc->front())) {
                            fr.next_char();
                            xs += m_nc->front();
                            // state remains the same
                        } else if (*m_nc == '.') {
                            fr.next_char();
                            xs += '.';
                            state = AFTER_DECIMAL_DOT;
                        } else {
                            state = DONE;
                        }
                    } else {
                        state = DONE;
                    }
                    break;
                case MAYBE_DECIMAL_DOT:
                    if (m_nc && *m_nc == '.') {
                        fr.next_char();
                        xs += '.';
                        state = AFTER_DECIMAL_DOT;
                    } else {
                        state = DONE;
                    }
                    break;
                case AFTER_DECIMAL_DOT:
                    if (m_nc && isdigit(m_nc->front())) {
                        fr.next_char();
                        xs += m_nc->front();
                    } else {
                        if (xs.back() == '.') {
                            report_error();
                            return {};
                        } else {
                            state = DONE;
                        }
                    }
                    break;
                default:
                    UL_UNREACHABLE;
            }  // switch state
        } while (state != DONE);
        return ast.new_token(move(xs), ast::Token::NUMBER);
    }

    maybe<ast::Expr*> read_sym()
    {
        string xs;
        for (;;) {
            auto mc = fr.peek_char();
            if (!mc || !is_symbol_char(*mc))
                break;
            fr.next_char();
            xs.append(BE(*mc));
        }
        if (xs.empty()) {
            report_error();
            return {};
        }
        return ast.new_token(move(xs), ast::Token::STRING);
    }

    void report_error(const string& msg)
    {
        error = StrFormat("%s %s:%d:%d.", msg, fr.filename, fr.line(), fr.col());
    }
    void report_error()
    {
        auto m_nc = fr.peek_char();
        if (m_nc) {
            report_error(StrFormat("Unexpected char %s in file", to_descriptive_string(*m_nc)));
        } else {
            if (auto me = fr.maybe_get_error()) {
                error = *me;
            } else if (fr.is_eof()) {
                report_error("Unexpected end of file at");
            } else {
                report_error("Not receiving any more characters after");
            }
        }
    }
    void report_missing_char(char c, CharLC unmatched)
    {
        error =
            StrFormat("Missing character '%c' at %s:%d:%d to match '%c' at %d:%d.", c, fr.filename,
                      fr.line(), fr.col(), unmatched.c, unmatched.line, unmatched.col);
    }
};

namespace AstBuilder {
either<string, vector<ast::Expr*>> parse_filereader_into_ast(FileReader& fr, Ast& ast)
{
    return AstBuilderImpl{fr, ast}.run();
}
}  // namespace AstBuilder

}  // namespace forrest
