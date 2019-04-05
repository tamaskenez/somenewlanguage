#include "cppgen.h"

#include "absl/strings/str_format.h"
#include "ul/usual.h"

#include "util/maybe.h"
#include "util/utf.h"

#include "ast.h"

namespace forrest {

using std::get_if;
using std::holds_alternative;
using std::in_place_type;

using absl::StrFormat;

using namespace ul;

const char* node_type_name(const Expr& e)
{
    struct Visitor
    {
        auto operator()(const TupleNode&) { return "vector"; }
        auto operator()(const StrNode& x) { return "string"; }
        auto operator()(const SymLeaf& x) { return "symbol"; }
        auto operator()(const NumLeaf& x) { return "number"; }
        auto operator()(const CharLeaf& x) { return "char"; }
        auto operator()(const VoidLeaf& x) { return "void"; }
        auto operator()(const ApplyNode& x) { return "apply"; }
        auto operator()(const QuoteNode& x) { return "quote"; }
    };
    return std::visit(Visitor{}, e);
}

string to_string(ExprRef e)
{
    struct Visitor
    {
        string s;
        void operator()(const TupleNode& x, char open_char = '(', char close_char = ')')
        {
            s += open_char;
            bool first = true;
            for (auto& i : x.xs) {
                if (first) {
                    first = false;
                    s += ' ';
                }
                visit(*this, *i);
            }
            s += close_char;
        }
        void operator()(const StrNode& x)
        {
            s += '"';
            for (auto c : x.xs) {
                if (!iscntrl(c) && is_ascii_utf8_byte(c))
                    s += c;
                else {
                    s += StrFormat("\\U+%02X;", c);
                }
            }
            s += '"';
        }
        void operator()(const SymLeaf& x)
        {
            s += StrFormat("%s", (const char*)(x.sym->first.c_str()));
        }
        void operator()(const NumLeaf& x) { s += StrFormat("%s", x.x); }
        void operator()(const CharLeaf& x)
        {
            s += StrFormat("%s", utf32_to_descriptive_string(x.x));
        }
        void operator()(const VoidLeaf& x) {}
        void operator()(const ApplyNode& x) { (*this)(*x.tuple_ref, '{', '}'); }
        void operator()(const QuoteNode& x)
        {
            s += '`';
            visit(*this, *x.expr_ref);
        }
    } visitor;

    if (auto str = get_if<StrNode>(e)) {
        return str->xs;
    }
    visit(visitor, *e);
    return visitor.s;
}

maybe<Expr> apply_print(Ast& ast, const vector<ExprRef>& xs)
{
    string s;
    FOR (i, 1, < ~xs) {
        s += to_string(xs[i]);
    }

    // todo encode to c string literal.
    printf("%s", s.c_str());
    return *ast.voidleaf();
}

maybe<Expr> eval_apply(Ast& ast, const vector<ExprRef>& xs)
{
    if (xs.empty()) {
        fprintf(stderr, "ERROR: applying empty vector.\n");
        return {};
    }
    auto pf = get_if<SymLeaf>(xs.front());
    if (!pf) {
        fprintf(stderr, "ERROR: apply vector's first element is not a symbol but %s.\n",
                node_type_name(*xs.front()));
    }
    auto& kv = *(pf->sym);
    auto& sym_name = kv.first;
    if (sym_name == "print") {
        return apply_print(ast, xs);
    } else {
        fprintf(stderr, "ERROR: Unknown function: %s.\n", sym_name.c_str());
        return {};
    }
}

enum ApplyChoice
{
    apply_false,
    apply_true
};

maybe<Expr> eval(Ast& ast, ExprRef e)
{
    struct Visitor
    {
        Ast& ast;
        const ExprRef expr_ref;
        Visitor(Ast& ast, ExprRef expr_ref) : ast(ast), expr_ref(expr_ref) {}
        maybe<Expr> operator()(const TupleNode& e, ApplyChoice apply_choice = apply_false)
        {
            vector<ExprRef> ys;
            ys.reserve(~e.xs);
            for (auto x : e.xs) {
                maybe<Expr> my = eval(ast, x);
                if (!my)
                    return {};
                if (!holds_alternative<VoidLeaf>(*my)) {
                    ast.storage.emplace_back(move(*my));
                    ys.push_back(&ast.storage.back());
                }
            }
            if (apply_choice == apply_true) {
                return eval_apply(ast, ys);
            } else {
                return Expr(in_place_type<TupleNode>, move(ys));
            }
        }
        maybe<Expr> operator()(const StrNode& x) { return *expr_ref; }
        maybe<Expr> operator()(const SymLeaf& x) { return *expr_ref; }
        maybe<Expr> operator()(const NumLeaf& x) { return *expr_ref; }
        maybe<Expr> operator()(const CharLeaf& x) { return *expr_ref; }
        maybe<Expr> operator()(const VoidLeaf& x) { return *expr_ref; }
        maybe<Expr> operator()(const ApplyNode& x) { return (*this)(*x.tuple_ref, apply_true); }
        maybe<Expr> operator()(const QuoteNode& x) { return *x.expr_ref; }
    };
    return visit(Visitor{ast, e}, *e);
}

bool cppgen(Ast& ast, const CommandLineOptions& clo)
{
    bool ok = true;

    for (auto& e : ast.top_level_exprs) {
        auto r = eval(ast, e);
        if (r) {
            dump(&*r);
        } else {
            ok = false;
        }
    }
    return ok;
}

}  // namespace forrest
