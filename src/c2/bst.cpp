#include "bst.h"

#include "absl/strings/str_format.h"
#include "ast_syntax.h"
#include "util/utf.h"

namespace forrest {

using absl::PrintF;
using absl::StrFormat;

const bst::Expr* process_ast(ast::Expr* e)
{
    if (auto l = get_if<ast::List>(e)) {
        if (l->xs.empty()) {
            return &bst::EXPR_EMPTY_TUPLE;
        }
        if (l->fnapp) {
            if (auto t = get_if<ast::Token>(l->xs.front())) {
                if (t->kind == ast::Token::STRING && t->x == LAMBDA_ABSTRACTION_KEYWORD) {
                    CHECK(~l->xs == 3, "Lambda abstraction needs 2 arguments.");
                    auto pars = get_if<ast::List>(l->xs[1]);
                    CHECK(pars && !pars->fnapp,
                          "Lambda abstraction first argument must be a list of parameters.");
                    vector<bst::FnPar> fnpars;
                    fnpars.reserve(~pars->xs);
                    auto either_fnpars = &fnpars;
                    for (auto par : pars->xs) {
                        auto p = get_if<ast::Token>(par);
                        CHECK(p);
                        if (p->kind == ast::Token::QUOTED_STRING) {
                            CHECK(is_parameter_name(p->x));
                            either_fnpars->push_back(bst::FnPar{p->x});
                        } else if (p->kind == ast::Token::STRING) {
                            if (p->x == IGNORED_PARAMETER) {
                                either_fnpars->push_back(bst::FnPar{{}});
                            } else {
                                UL_UNREACHABLE;
                            }
                        } else {
                            UL_UNREACHABLE;
                        }
                    }
                    auto body = process_ast(l->xs[2]);
                    return new bst::Fn(move(fnpars), body);
                }
            }
            auto it = l->xs.begin();

            auto head = process_ast(*it);
            ++it;
            vector<bst::FnArg> args;
            for (; it != l->xs.end(); ++it) {
                args.emplace_back(process_ast(*it));
            }
            return new bst::Fnapp(head, move(args));
        } else {
            // Simple list, not a function application.
            assert(!l->fnapp);
            vector<const bst::Expr*> ys;
            ys.reserve(~l->xs);
            for (auto x : l->xs) {
                ys.push_back(process_ast(x));
            }
            return new bst::Tuple(move(ys));
        }
    } else if (auto t = get_if<ast::Token>(e)) {
        switch (t->kind) {
            case ast::Token::STRING:
                if (auto m = maybe_builtin_from_cstring(CSTR t->x)) {
                    return new bst::Builtin(*m);
                } else {
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
    UL_UNREACHABLE;
    return nullptr;
}  // namespace forrest

void dump(bst::Expr* expr)
{
    struct Visitor
    {
        string ind;
        void indent() { ind += " "; }
        void dedent() { ind.pop_back(); }
        void visit(const bst::Expr* e)
        {
            using namespace bst;
            switch (e->type) {
                case tString:
                    visit(*cast<String>(e));
                    break;
                case tNumber:
                    visit(*cast<Number>(e));
                    break;
                case tVarname:
                    visit(*cast<Varname>(e));
                    break;
                case tBuiltin:
                    visit(*cast<Builtin>(e));
                    break;
                case tInstr:
                    visit(*cast<Instr>(e));
                    break;
                case tFnapp:
                    visit(*cast<Fnapp>(e));
                    break;
                case tFn:
                    visit(*cast<Fn>(e));
                    break;
                case tTuple:
                    visit(*cast<Tuple>(e));
                    break;
            }
        }
        void visit(const bst::String& x)
        {
            string s;
            for (auto c : x.x) {
                if (is_ascii_utf8_byte(c) && (c == ' ' || isgraph(c))) {
                    s += c;
                } else {
                    s += StrFormat("\\U+%02X;", c);
                }
            }
            PrintF("%s\"%s\"\n", ind, s);
        }
        void visit(const bst::Number& x) { PrintF("%s#%s\n", ind, x.x); }
        void visit(const bst::Fnapp& x)
        {
            PrintF("%sApplication:\n", ind);
            indent();
            visit(x.fn_to_apply);
            for (auto e : x.args) {
                // TODO print e.name
                visit(e.value);
            }
            dedent();
        }
        void visit(const bst::Varname& x) { PrintF("%s%s\n", ind, x.x); }
        void visit(const bst::Builtin& x) { PrintF("%s<%s>\n", ind, to_cstring(x.x)); }
        void visit(const bst::Instr& x) { PrintF("%s%s\n", ind, to_string(x)); }
        void visit(const bst::Tuple& x)
        {  // TODO
            UL_UNREACHABLE;
        }
        void visit(const bst::Fn& x)
        {
            // TODO
            UL_UNREACHABLE;
        }
    };
    (Visitor{}).visit(expr);
}

void dump_dfs(const bst::Expr* expr)
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
}  // namespace forrest

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
