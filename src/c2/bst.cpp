#include "bst.h"

#include "absl/strings/str_format.h"
#include "ast_syntax.h"
#include "util/utf.h"

namespace forrest {

using absl::PrintF;
using absl::StrFormat;

/*
 if (auto x = get_if<bst::Tuple>(fn)) {
 return fn;
 } else if (auto x = get_if<bst::Vector>(fn)) {
 return fn;
 } else if (auto x = get_if<bst::String>(fn)) {
 } else if (auto x = get_if<bst::Number>(fn)) {
 } else if (auto x = get_if<bst::Fn>(fn)) {
 } else if (auto x = get_if<bst::Data>(fn)) {
 } else if (auto x = get_if<bst::Application>(fn)) {
 } else if (auto x = get_if<bst::Varname>(fn)) {
 }
 */

bst::Expr* process_ast(ast::Expr* e, Bst& bst)
{
    if (auto l = get_if<ast::List>(e)) {
        if (l->xs.empty()) {
            return &bst.exprs.emplace_back(in_place_type<bst::Tuple>, vector<bst::Expr*>{});
        }
        auto it = l->xs.begin();
        auto head = process_ast(*it, bst);
        ++it;
        vector<bst::Expr*> args, envargs;
        auto a = &args;
        for (; it != l->xs.end(); ++it) {
            auto x = *it;
            if (is_env_args_separator(x)) {
                if (a == &args) {
                    a = &envargs;
                    continue;
                } else {
                    CHECK(false, "Two env-args separator in function application");
                }
            }
            a->push_back(process_ast(x, bst));
        }
        if (args.empty() && envargs.empty()) {
            return head;
        }
        CHECK(!args.empty(), "Can't apply envargs without args.");
        return &bst.exprs.emplace_back(in_place_type<bst::Application>, head, move(args),
                                       move(envargs));
        UL_UNREACHABLE;
    } else if (auto t = get_if<ast::Token>(e)) {
        switch (t->kind) {
            case ast::Token::STRING:
                return &bst.exprs.emplace_back(in_place_type<bst::Varname>, t->x);
            case ast::Token::QUOTED_STRING:
                return &bst.exprs.emplace_back(in_place_type<bst::String>, t->x);
            case ast::Token::NUMBER:
                return &bst.exprs.emplace_back(in_place_type<bst::Number>, t->x);
            default:
                UL_UNREACHABLE;
        }
    }
    UL_UNREACHABLE;
    return nullptr;
}

void dump(bst::Expr* expr)
{
    struct Visitor
    {
        string ind;
        void indent() { ind += " "; }
        void dedent() { ind.pop_back(); }
        void operator()(const bst::Tuple& x)
        {
            if (x.xs.empty()) {
                PrintF("%sTuple ()\n", ind);
            } else {
                PrintF("%sTuple (\n", ind);
                indent();
                for (auto expr_ptr : x.xs) {
                    visit(*this, *expr_ptr);
                }
                dedent();
                PrintF("%s)\n", ind);
            }
        }
        void operator()(const bst::Vector& x)
        {
            PrintF("%sVector (\n", ind);
            indent();
            for (auto expr_ptr : x.xs) {
                visit(*this, *expr_ptr);
            }
            dedent();
            PrintF("%s)\n", ind);
        }
        void operator()(const bst::String& x)
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
        void operator()(const bst::Number& x) { PrintF("%s#%s\n", ind, x.x); }
        void operator()(const bst::Fn& x)
        {
            PrintF("%sFn:\n", ind);
            indent();
            PrintF("%s", ind);
            for (auto& p : x.pars) {
                PrintF("%s ", p);
            }
            PrintF("\n%s", ind);
            for (auto& p : x.envpars) {
                PrintF("%s ", p);
            }
            PrintF("\n");
            visit(*this, *x.body);
            dedent();
        }
        void operator()(const bst::Data& x)
        {
            PrintF("%sData:\n", ind);
            indent();
            visit(*this, *x.def);
            dedent();
        }
        void operator()(const bst::Application& x)
        {
            PrintF("%sApplication:\n", ind);
            indent();
            visit(*this, *(x.head));
            for (auto e : x.args) {
                visit(*this, *e);
            }
            if (!x.envargs.empty()) {
                dedent();
                PrintF("%sEnvargs:\n", ind);
                indent();
                for (auto e : x.envargs) {
                    visit(*this, *e);
                }
            }
            dedent();
        }
        void operator()(const bst::Varname& x) { PrintF("%s%s\n", ind, x.x); }
    };
    visit(Visitor{}, *expr);
}

void dump_dfs(bst::Expr* expr)
{
    struct Visitor
    {
        vector<string> exprs;
        string ind;
        void indent() { ind += " "; }
        void dedent() { ind.pop_back(); }
        string last_index() { return StrFormat("$%d", ~exprs - 1); }
        string operator()(const bst::Tuple& x)
        {
            if (x.xs.empty()) {
                return "()";
            } else {
                string r = "(Tuple";
                for (auto expr_ptr : x.xs) {
                    r += " ";
                    r += visit(*this, *expr_ptr);
                }
                r += ")";
                exprs.push_back(move(r));
                return last_index();
            }
        }
        string operator()(const bst::Vector& x)
        {
            if (x.xs.empty()) {
                return "()";
            } else {
                string r = "(Vector";
                for (auto expr_ptr : x.xs) {
                    r += " ";
                    r += visit(*this, *expr_ptr);
                }
                r += ")";
                exprs.push_back(move(r));
                return last_index();
            }
        }
        string operator()(const bst::String& x)
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
        string operator()(const bst::Number& x) { return StrFormat("#%s", x.x); }
        string operator()(const bst::Fn& x)
        {
            string s = "(fn (";
            for (auto& p : x.pars) {
                s += " ";
                s += p;
            }
            s += ") (";
            for (auto& p : x.envpars) {
                s += " ";
                s += p;
            }
            s += ") ";
            s += visit(*this, *x.body);
            exprs.push_back(s);
            return last_index();
        }
        string operator()(const bst::Data& x)
        {
            exprs.push_back(StrFormat("(Data %s %s)", x.name, visit(*this, *x.def)));
            return last_index();
        }
        string operator()(const bst::Application& x)
        {
            string s = StrFormat("(%s", visit(*this, *(x.head)));
            for (auto e : x.args) {
                s += " ";
                s += visit(*this, *e);
            }
            if (!x.envargs.empty()) {
                s += " ";
                s += ENV_ARGS_SEPARATOR;
                for (auto e : x.envargs) {
                    s += " ";
                    s += visit(*this, *e);
                }
            }
            s += ")";
            exprs.push_back(s);
            return last_index();
        }
        string operator()(const bst::Varname& x) { return x.x; }
    };
    Visitor v;
    string result = visit(v, *expr);
    FOR (i, 0, < ~v.exprs) {
        PrintF("$%d = %s\n", i, v.exprs[i]);
    }
    PrintF("%s\n", result);
}

}  // namespace forrest
