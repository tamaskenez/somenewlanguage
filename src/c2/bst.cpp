#include "bst.h"

#include "absl/strings/str_format.h"
#include "ast_syntax.h"
#include "util/utf.h"

namespace forrest {

using absl::PrintF;
using absl::StrFormat;

const bst::Expr* process_ast(ast::Expr* e)
{
    using namespace bst;
    if (auto l = get_if<ast::List>(e)) {
        if (l->xs.empty()) {
            CHECK(!l->fnapp);
            return &bst::EXPR_EMPTY_TUPLE;
        }
        vector<const bst::Expr*> ys;
        if (l->fnapp) {
            maybe<ast::Builtin> m_bi_head;
            if (auto t = get_if<ast::Token>(l->xs[0])) {
                if (t->kind == ast::Token::STRING) {
                    m_bi_head = maybe_builtin_from_cstring(CSTR t->x);
                }
            }
            if (m_bi_head) {
                ys.reserve(~l->xs - 1);
                FOR (i, 1, < ~l->xs) {
                    ys.push_back(process_ast(l->xs[i]));
                }
                return new Builtin(*m_bi_head, new Tuple(move(ys)));
            } else {
                ys.reserve(~l->xs);
                for (auto x : l->xs) {
                    ys.push_back(process_ast(x));
                }
                return new Builtin(ast::Builtin::FNAPP, new Tuple(move(ys)));
            }
        }  // if fnapp
        ys.reserve(~l->xs);
        for (auto x : l->xs) {
            ys.push_back(process_ast(x));
        }
        return new Tuple(move(ys));
    }  // if list
    auto t = &get<ast::Token>(*e);
    switch (t->kind) {
        case ast::Token::STRING: {
            auto m = maybe_builtin_from_cstring(CSTR t->x);
            CHECK(!m);
            return new bst::Varname(t->x);
        }
        case ast::Token::QUOTED_STRING:
            return new bst::String(t->x);
        case ast::Token::NUMBER:
            return new bst::Number(t->x);
        default:
            UL_UNREACHABLE;
    }
}

void dump(const bst::Expr* e, int indent = 0)
{
    using namespace bst;
    switch (e->type) {
        case tString: {
            string s;
            for (auto c : cast<String>(e)->x) {
                if (is_ascii_utf8_byte(c) && (c == ' ' || isgraph(c))) {
                    s += c;
                } else {
                    s += StrFormat("\\U+%02X;", c);
                }
            }
            PrintF("%s\"%s\"\n", string(indent, ' '), s);
        } break;
        case tNumber:
            PrintF("%s#%s\n", string(indent, ' '), cast<Number>(e)->x);
            break;
        case tVarname:
            PrintF("%s%s\n", string(indent, ' '), cast<Varname>(e)->x);
            break;
        case tBuiltin: {
            auto bi = cast<Builtin>(e);
            PrintF("%s(%s", string(indent, ' '), to_cstring(bi->head));
            if (!bi->xs->xs.empty()) {
                PrintF("\n");
                for (auto x : bi->xs->xs) {
                    dump(x.x, indent + 1);
                }
            }
            PrintF(")\n");
        } break;
        case tTuple: {
            auto t = cast<Tuple>(e);
            if (t->xs.empty()) {
                PrintF("()\n");
            } else {
                PrintF("%s(\n", string(indent, ' '));
                for (auto x : t->xs) {
                    dump(x.x, indent + 1);
                }
                PrintF("%s)\n", string(indent, ' '));
            }
        } break;
        case tInstr:
            PrintF("%s%s\n", string(indent, ' '), to_string(*cast<Instr>(e)));
            break;
        case tFnapp:
        case tFn:
            UL_UNREACHABLE;
            break;
    }
}
/*
void dump_dfs(const bst::Expr* expr, int indent=0)
{
    struct Visitor
    {
        vector<string> exprs;
        string ind;
        void indent() { ind += " "; }
        void dedent() { ind.pop_back(); }
        string last_index() { return StrFormat("$%d", ~exprs - 1); }
        string visit(const bst::Expr* e)
        {
            using namespace bst;
            switch (e->type) {
                case tString:
                    return visit(*cast<String>(e));
                case tNumber:
                    return visit(*cast<Number>(e));
                case tVarname:
                    return visit(*cast<Varname>(e));
                case tBuiltin:
                    return visit(*cast<Builtin>(e));
                case tInstr:
                    return visit(*cast<Instr>(e));
                case tFnapp:
                    return visit(*cast<Fnapp>(e));
                case tFn:
                    return visit(*cast<Fn>(e));
                case tTuple:
                    return visit(*cast<Tuple>(e));
            }
        }
        string visit(const bst::String& x)
        {
            string s;
            for (auto c : x.x) {
                if (is_ascii_utf8_byte(c) && (c == ' ' || isgraph(c))) {
                    s += c;
                } else {
                    s += StrFormat("\\U+%02X;", c);
                }
            }
            return StrFormat("\"%s\"", s);
        }
        string visit(const bst::Number& x) { return StrFormat("#%s", x.x); }
        string visit(const bst::Fnapp& x)
        {
            string s;
            s = StrFormat("(%s", visit(x.fn_to_apply));
            bool need_space = true;
            for (auto e : x.args) {
                if (need_space) {
                    s += " ";
                } else {
                    need_space = true;
                }
                s += visit(e.value);
            }
            s += ")";
            exprs.push_back(s);
            return last_index();
        }
        string visit(const bst::Varname& x) { return x.x; }
        string visit(const bst::Builtin& x) { return to_cstring(x.x); }
        string visit(const bst::Instr& x) { return to_string(x); }
        string visit(const bst::Tuple& x)
        {
            string s = "[";
            bool need_space = false;
            for (auto e : x.xs) {
                if (need_space) {
                    s += " ";
                } else {
                    need_space = true;
                }
                s += visit(e.x);
            }
            s += "]";
            return s;
        }
        string visit(const bst::Fn& x)
        {
            string s = "(fn [";
            bool need_space = false;
            for (auto p : x.pars) {
                if (need_space) {
                    s += " ";
                } else {
                    need_space = true;
                }
                s += p.name;
            }
            s += "] " + visit(x.body) + ")";
            return s;
        }
    };
    Visitor v;
    string result = v.visit(expr);
    FOR (i, 0, < ~v.exprs) {
        PrintF("$%d = %s\n", i, v.exprs[i]);
    }
    PrintF("%s\n", result);
}
*/

namespace bst {

string to_string(const Instr& x)
{
    switch (x.opcode) {
        case Instr::OP_READ_VAR:
            return StrFormat("READ_VAR %s", cast<Varname>(x.arg0)->x);
        case Instr::OP_CALL_FUNCTION:
            UL_UNREACHABLE;
            break;
    }
    UL_UNREACHABLE;
    return {};
}

vector<NamedExpr> vector_expr_to_vector_namedexpr(const vector<const Expr*>& xs)
{
    vector<NamedExpr> ys;
    ys.reserve(~xs);
    for (auto p : xs) {
        ys.emplace_back(p);
    }
    return ys;
}

}  // namespace bst

maybe<const bst::Expr*> lookup_by_name(const bst::Tuple* t, const string& n)
{
    for (auto& x : t->xs) {
        if (x.n == n) {
            return x.x;
        }
    }
    return {};
}

}  // namespace forrest
