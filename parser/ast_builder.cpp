#include "ast_builder.h"

#include <unordered_map>

#include "absl/strings/str_format.h"
#include "ul/usual.h"
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

class AstBuilderImpl : public AstBuilder
{
    FileReader& fr;
    string error;

    // Stack of children-vectors of parents, back() is the active one.
    // If empty, we're on top level, next expression will be a root.
    vector<vector<ExprRef>*> active_parent_stack;

    struct CharLC
    {
        char c;
        int line, col;
    };

public:
    AstBuilderImpl(FileReader& fr) : fr(fr) {}

    virtual bool parse(Ast& ast) override
    {
        fr.skip_whitespace();
        return read_expr(ast);
    }

private:
    // Add to storage, add to active parent (or open new root) and push it to parent stack.
    void push_new_vec_node_onto_stack(bool apply, Ast& ast)
    {
        ast.storage.emplace_back(in_place_type<TupleNode>, apply);
        add_storageback_to_active_parent_vec(ast);
        TupleNode& exprs_node = get<TupleNode>(ast.storage.back());
        active_parent_stack.push_back(&exprs_node.xs);
    }

    void pop_vec_node_from_stack()
    {
        assert(!active_parent_stack.empty());
        active_parent_stack.pop_back();
    };

    void add_storageback_to_active_parent_vec(Ast& ast)
    {
        assert(!ast.storage.empty());
        ExprRef expr_ref = &ast.storage.back();
        if (active_parent_stack.empty()) {
            // Add new top-level expression.
            ast.top_level_exprs.push_back(expr_ref);
        } else {
            active_parent_stack.back()->push_back(expr_ref);
        }
    }

    // Whitespace skipped before this.
    bool read_expr(Ast& ast)
    {
        if (!fr.read_ahead_at_least_1()) {
            report_error();
            return false;
        }
        if (fr.attempt_wora(OPEN_VEC_CHAR)) {  // Start Exprs
            return read_vec(false, ast);
        } else if (fr.attempt_wora(OPEN_AVEC_CHAR)) {  // Start function application.
            return read_vec(true, ast);
        } else if (fr.attempt_wora(STRING_QUOTE_CHAR)) {  // Start AsciiStr/Str
            return read_str(ast);
        } else if (fr.attempt_wora(UTFCHAR_PREFIX)) {  // Start utfchar
            auto mc = read_utf8char();
            if (!mc) {
                if (error.empty()) {
                    report_error();
                }
                return false;
            }
            auto mcp = mc->code_point();
            if (!mcp) {
                // This actually cannot happen since FileReader doesn't let through invalid
                // UTF8.
                report_error("Invalid UTF8 character literal at");
                return false;
            }
            ast.storage.emplace_back(in_place_type<CharLeaf>, *mcp);
            add_storageback_to_active_parent_vec(ast);
            return true;
        } else if (fr.peek_wora(
                       [](Utf8Char c) { return c == '-' || c == '+' || isdigit(c.front()); })) {
            return read_num(ast);
        } else {
            return read_sym(ast);
        }
        UL_UNREACHABLE;
    }

    bool read_vec(bool apply, Ast& ast)
    {
        char open_char, close_char;
        if (apply) {
            open_char = OPEN_AVEC_CHAR;
            close_char = CLOSE_AVEC_CHAR;
        } else {
            open_char = OPEN_VEC_CHAR;
            close_char = CLOSE_VEC_CHAR;
        }
        CharLC open_char_lc{open_char, fr.line(), fr.col()};
        push_new_vec_node_onto_stack(apply, ast);
        for (;;) {
            fr.skip_whitespace();
            if (!fr.read_ahead_at_least_1()) {
                report_missing_char(close_char, open_char_lc);
                return false;
            }
            if (fr.attempt_wora(close_char)) {
                pop_vec_node_from_stack();
                return true;
            }
            if (!read_expr(ast))
                return false;
            // An item must be followed by whitespace or closing char.
            if (!fr.peek_wora(
                    [close_char](Utf8Char c) { return c == close_char || isspace(c.front()); })) {
                report_error();
                return false;
            }
        }
        return true;
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

    bool read_str(Ast& ast)
    {
        CharLC begin_char{STRING_QUOTE_CHAR, fr.line(), fr.col()};
        string xs;
        for (;;) {
            auto m_nc = read_utf8char();
            if (!m_nc) {
                if (error.empty()) {
                    report_missing_char(STRING_QUOTE_CHAR, begin_char);
                }
                return false;
            }
            if (*m_nc == STRING_QUOTE_CHAR) {
                ast.storage.emplace_back(in_place_type<StrNode>, move(xs));
                add_storageback_to_active_parent_vec(ast);
                return true;
            }
            xs.append(BE(*m_nc));
        }
    }

    bool read_num(Ast& ast)
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
                        return false;
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
                    return false;
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
                            return false;
                        } else {
                            state = DONE;
                        }
                    }
                    break;
                default:
                    UL_UNREACHABLE;
            }  // switch state
        } while (state != DONE);
        ast.storage.emplace_back(in_place_type<NumLeaf>, move(xs));
        add_storageback_to_active_parent_vec(ast);
        return true;
    }

    bool read_sym(Ast& ast)
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
            return false;
        }
        ast.storage.emplace_back(in_place_type<SymLeaf>, ast.get_or_create_symbolref(move(xs)));
        add_storageback_to_active_parent_vec(ast);
        return true;
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
            if (fr.is_error()) {
                error = fr.get_error();
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

unique_ptr<AstBuilder> AstBuilder::new_(FileReader& fr)
{
    return make_unique<AstBuilderImpl>(fr);
}

}  // namespace forrest
