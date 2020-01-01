#include "bst.h"

#include "absl/strings/str_format.h"
#include "ast_syntax.h"
#include "util/utf.h"

namespace forrest {

using absl::PrintF;
using absl::StrFormat;

string to_string(StringTree* st, bool oneline, int indent)
{
    CHECK(!oneline || indent == 0);
    string text = ((st->text.empty() && st->children.empty()) ? string("\"\"") : st->text);
    string s = string(indent, ' ') + text;
    if (st->children.empty()) {
        if (!oneline) {
            s += "\n";
        }
        return s;
    }
    if (oneline || st->only_leaf_children()) {
        s += text.empty() ? "{" : " {";
        FOR (i, 0, < ~st->children) {
            if (i > 0) {
                s += ' ';
            }
            s += to_string(st->children[i], true);
        }
        s += '}';
        if (!oneline) {
            s += '\n';
        }
        return s;
    }
    s += " {\n";
    for (auto c : st->children) {
        s += to_string(c, false, indent + 2);
    }
    s += string(indent, ' ') + "}\n";
    return s;
}

const bst::Expr* process_ast(ast::Expr* e, const bst::LexicalScope* ls)
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
                switch (*m_bi_head) {
                    case ast::Builtin::DEF: {
                        // (def name body)
                        CHECK(~l->xs == 3);
                        auto l0 = get<ast::Token>(*l->xs[1]);
                        CHECK(l0.kind == ast::Token::QUOTED_STRING);
                        auto name = l0.x;
                        // 'def' doesn't introduce new lexical scope.
                        auto body = process_ast(l->xs[2], ls);
                        return new Def(name, body);
                    }
                    case ast::Builtin::FN: {
                        // (fn pars body) where pars is list of qstrings
                        CHECK(~l->xs == 3);
                        auto l0 = get<ast::List>(*l->xs[1]);
                        CHECK(!l0.fnapp);
                        vector<FnPar> fnpars;
                        CHECK(!l0.xs.empty());
                        vector<const Variable*> vars;
                        for (auto par : l0.xs) {
                            CHECK(holds_alternative<ast::Token>(*par));
                            auto par2 = get<ast::Token>(*par);
                            CHECK(par2.kind == ast::Token::QUOTED_STRING);
                            auto v = new Variable(par2.x);
                            vars.emplace_back(v);
                            fnpars.emplace_back(v);
                        }
                        LexicalScope new_ls(ls, move(vars));
                        return new Fn(fnpars, process_ast(l->xs[2], &new_ls));
                    }
                    case ast::Builtin::LET: {
                        // (let name body)
                        CHECK(~l->xs == 4);
                        auto l0 = get<ast::Token>(*l->xs[1]);
                        CHECK(l0.kind == ast::Token::QUOTED_STRING);
                        auto name = l0.x;
                        LexicalScope new_ls(ls, vector<const Variable*>{new Variable(name)});
                        auto value = process_ast(l->xs[2], &new_ls);
                        auto body = process_ast(l->xs[3], &new_ls);
                        return new Let(name, value, body);
                    }
                    case ast::Builtin::FNAPP:  // fnapp is implicit.
                    default:
                        UL_UNREACHABLE;
                }
                UL_UNREACHABLE;
            } else {
                auto head = process_ast(l->xs[0], ls);
                ys.reserve(~l->xs - 1);
                FOR (i, 1, < ~l->xs) {
                    ys.push_back(process_ast(l->xs[i], ls));
                }
                return new Fnapp(head, move(ys));
            }
        } else {
            assert(!l->fnapp);
            ys.reserve(~l->xs);
            for (auto x : l->xs) {
                ys.push_back(process_ast(x, ls));
            }
            return new Tuple(move(ys));
        }
    } else {
        // Not a list, must be a token.
        auto t = &get<ast::Token>(*e);
        switch (t->kind) {
            case ast::Token::STRING: {
                auto m = maybe_builtin_from_cstring(CSTR t->x);
                CHECK(!m);
                if (auto m_v = ls->try_resolve_variable_name(t->x)) {
                    return *m_v;
                } else {
                    return new ToplevelVariableName(t->x);
                }
            }
            case ast::Token::QUOTED_STRING:
                return new bst::String(t->x);
            case ast::Token::NUMBER:
                return new bst::Number(t->x);
            default:
                UL_UNREACHABLE;
        }
    }
}
#if 0
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

#endif
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

#if 0
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
#endif

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
