#include <string>
#include <unordered_map>
#include <vector>

#include "absl/strings/str_format.h"
#include "ast.h"
#include "ast_builder.h"
#include "ast_syntax.h"
#include "bst.h"
#include "command_line.h"
#include "consts.h"
#include "cppgen.h"
#include "cst.h"
#include "ul/check.h"
#include "ul/string.h"
#include "ul/usual.h"
#include "util/arena.h"
#include "util/filereader.h"
#include "util/log.h"

namespace forrest {

using absl::PrintF;
using std::make_unique;
using std::move;
using std::string;
using std::vector;

using std::get_if;

using namespace ul;

static const char* const USAGE_TEXT =
    R"~~~~(%1$s: parse forrest-AST text file
Usage: %1$s --help
       %1$s <input-files> [--cpp-out <filename>]
)~~~~";

// Add parsed data to ast.
maybe<vector<ast::Expr*>> parse_fast_file_add_to_ast(const string& filename, Ast& ast)
{
    auto lr = FileReader::new_(filename);
    if (is_left(lr)) {
        report_error(left(lr));
        return {};
    }
    // Call AstBuilder with new FileReader.
    auto plr = AstBuilder::parse_filereader_into_ast(right(lr), ast);
    if (is_left(plr)) {
        report_error(left(plr));
        return {};
    }
    return right(plr);
}

int run_fc_with_parsed_command_line(const CommandLineOptions& o)
{
    if (o.files.empty()) {
        fprintf(stderr, "No input files.\n");
        return EXIT_FAILURE;
    }

    bool ok = true;
    Ast ast;
    vector<ast::Expr*> top_level_exprs;
    for (auto& f : o.files) {
        auto m_exprs = parse_fast_file_add_to_ast(f, ast);
        if (m_exprs) {
            absl::PrintF("Compiled %s\n", f);
            top_level_exprs.insert(top_level_exprs.end(), BE(*m_exprs));
        } else {
            ok = false;
        }
    }
    if (!ok) {
        return EXIT_FAILURE;
    }
    // Dump top level expressions.
    FOR (i, 0, < ~top_level_exprs) {
        PrintF("-- TOP LEVEL #%d\n", i);
        dump(top_level_exprs[i]);
    }
    Bst bst;
    vector<bst::Expr*> top_level_bexprs;
    for (auto x : top_level_exprs) {
        top_level_bexprs.push_back(process_ast(x, bst));
    }
    for (auto x : top_level_bexprs) {
        dump_dfs(x);
    }
    // Process top-level expressions.
    bst::Env toplevel_env;
    for (auto x : top_level_bexprs) {
        if (auto app = get_if<bst::Fnapp>(x)) {
            if (auto bi = get_if<bst::Builtin>(app->head)) {
                switch (bi->x) {
                    case Builtin::DEF: {
                        // TODO errmsg
                        CHECK(~app->args == 2);
                        CHECK(app->envargs.empty());
                        auto s = get_if<bst ::String>(app->args[0]);
                        CHECK(s && !s->x.empty());
                        toplevel_env.add_local(s->x, bst::LocalVar{app->args[1], bst::IMMUTABLE});
                    } break;
                    default:
                        UL_UNREACHABLE;
                }
            } else if (auto vn = get_if<bst::Varname>(app->head)) {
                // TODO
                UL_UNREACHABLE;
            } else {
                // TODO
                UL_UNREACHABLE;
            }
        } else {
            // TODO
            UL_UNREACHABLE;
        }
    }

    const string ENTRY_POINT = "main";
    auto m_ep = toplevel_env.lookup_local(ENTRY_POINT);
    CHECK(m_ep, "No entry point found");
    auto ep = m_ep->x;
    auto a =
        bst::Fnapp{ep, vector<const bst::Expr*>{&bst::FNAPP_TUPLE_0}, vector<const bst::Expr*>{}};
    compile(&a, bst, &toplevel_env);
    /*
        Shell shell;
        for (auto x : top_level_exprs) {
            auto lr = shell.eval(x);
            if (is_left(lr)) {
                fprintf(stderr, "Error: %s\n", left(lr).msg.c_str());
                return EXIT_FAILURE;
            }
        }

        for (auto& kv : shell.symbols) {
            auto& s = kv.first;
            auto& v = kv.second;
            int a = 3;
        }
    */
    return EXIT_SUCCESS;
}

int c2main(int argc, const char** argv)
{
    g_log.program_name = PROGRAM_NAME;

    auto m_cl = parse_command_line(argc, argv);
    if (!m_cl) {
        return EXIT_FAILURE;
    }
    auto& cl = *m_cl;
    int result;
    if (cl.help) {
        PrintF(USAGE_TEXT, PROGRAM_NAME);
        result = EXIT_SUCCESS;
    } else {
        result = run_fc_with_parsed_command_line(cl);
    }
    return result;
}

}  // namespace forrest

int main(int argc, const char* argv[])
{
    try {
        return forrest::c2main(argc, argv);
    } catch (std::exception& e) {
        fprintf(stderr, "Aborting, exception: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "Aborting, unknown exception\n");
    }
    return EXIT_FAILURE;
}
