#include "bst.h"

#include "absl/strings/str_format.h"
#include "ast_syntax.h"
#include "util/utf.h"

namespace forrest {

using absl::PrintF;
using absl::StrFormat;

const bst::Expr* process_ast(ast::Expr* e, Bst& bst)
{
    if (auto l = get_if<ast::List>(e)) {
        if (l->xs.empty()) {
            return &bst::EXPR_EMPTY_LIST;
        }
        auto it = l->xs.begin();
        auto head = process_ast(*it, bst);
        ++it;
        vector<bst::FnArg> args, envargs;
        auto a = &args;
        for (; it != l->xs.end(); ++it) {
            auto x = *it;
            if (is_env_args_separator(x)) {
                if (a == &args) {
                    a = &envargs;
                    continue;
                } else {
                    CHECK(false, "Two env-args separators in function application");
                }
            }
            // TODO read name for named arguments.
            a->emplace_back(string{}, process_ast(x, bst));
        }
        CHECK(envargs.empty() || !args.empty(), "Can't apply envargs without args.");
        return &bst.exprs.emplace_back(in_place_type<bst::Fnapp>, head, move(args), move(envargs));
    } else if (auto t = get_if<ast::Token>(e)) {
        switch (t->kind) {
            case ast::Token::STRING:
                if (auto m = maybe_builtin_from_cstring(CSTR t->x)) {
                    return &bst.exprs.emplace_back(in_place_type<bst::Builtin>, *m);
                } else {
                    return &bst.exprs.emplace_back(in_place_type<bst::Varname>, t->x);
                }
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
}  // namespace forrest

void dump(bst::Expr* expr)
{
    struct Visitor
    {
        string ind;
        void indent() { ind += " "; }
        void dedent() { ind.pop_back(); }
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
        void operator()(const bst::Fnapp& x)
        {
            PrintF("%sApplication:\n", ind);
            indent();
            visit(*this, *(x.fn_to_apply));
            for (auto e : x.args) {
                // TODO print e.name
                visit(*this, *e.value);
            }
            if (!x.envargs.empty()) {
                dedent();
                PrintF("%sEnvargs:\n", ind);
                indent();
                for (auto e : x.envargs) {
                    // TODO print e.name
                    visit(*this, *e.value);
                }
            }
            dedent();
        }
        void operator()(const bst::Varname& x) { PrintF("%s%s\n", ind, x.x); }
        void operator()(const bst::Builtin& x) { PrintF("%s<%s>\n", ind, to_cstring(x.x)); }
        void operator()(const bst::Instr& x) { PrintF("%s%s\n", ind, to_string(x)); }
        void operator()(const bst::List& x)
        {  // TODO
            UL_UNREACHABLE;
        }
        void operator()(const bst::Fn& x)
        {
            // TODO
            UL_UNREACHABLE;
        }
    };
    visit(Visitor{}, *expr);
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
        string operator()(const bst::Fnapp& x)
        {
            string s;
            bool tuple_or_vector = false;
            bool need_space;
            if (auto vn = get_if<bst::Varname>(x.fn_to_apply)) {
                if (vn->x == "tuple" || vn->x == "vector") {
                    tuple_or_vector = true;
                    s = "[";
                    need_space = false;
                }
            }
            if (!tuple_or_vector) {
                s = StrFormat("(%s", visit(*this, *(x.fn_to_apply)));
                need_space = true;
            }
            for (auto e : x.args) {
                if (need_space) {
                    s += " ";
                } else {
                    need_space = true;
                }
                s += visit(*this, *e.value);
            }
            if (!x.envargs.empty()) {
                if (need_space) {
                    s += " ";
                }
                s += ENV_ARGS_SEPARATOR;
                for (auto e : x.envargs) {
                    s += " ";
                    s += visit(*this, *e.value);
                }
            }
            if (tuple_or_vector) {
                return s + "]";
            }
            s += ")";
            exprs.push_back(s);
            return last_index();
        }
        string operator()(const bst::Varname& x) { return x.x; }
        string operator()(const bst::Builtin& x) { return to_cstring(x.x); }
        string operator()(const bst::Instr& x) { return to_string(x); }
        string operator()(const bst::List& x)
        {
            // TODO
            UL_UNREACHABLE;
            return {};
        }
        string operator()(const bst::Fn& x)
        {
            // TODO
            UL_UNREACHABLE;
            return {};
        }
    };
    Visitor v;
    string result = visit(v, *expr);
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
            return StrFormat("READ_VAR %s", get<Varname>(*x.arg0).x);
    }
    UL_UNREACHABLE;
}

}  // namespace bst

}  // namespace forrest
