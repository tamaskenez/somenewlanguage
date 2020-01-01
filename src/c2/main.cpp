#include <map>
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
using std::map;
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
    vector<const bst::Expr*> top_level_bexprs;

    bst::LexicalScope ls_root;
    for (auto x : top_level_exprs) {
        top_level_bexprs.push_back(process_ast(x, &ls_root));
    }
    /*
    for (auto x : top_level_bexprs) {
        dump_dfs(x);
    }*/
    // Process top-level expressions.
    using namespace bst;
    map<string, pair<const Variable*, const Expr*>> toplevel_variables;
    using namespace bst;
    for (auto x : top_level_bexprs) {
        switch (x->type) {
            case tDef: {
                auto d = cast<Def>(x);
                CHECK(toplevel_variables.count(d->name) == 0);
                auto v = new bst::Variable(d->name);
                toplevel_variables[d->name] = make_pair(v, d->e);
            } break;

            default:
                UL_UNREACHABLE;
                break;
        }
    }

    const string ENTRY_POINT = "main";
    auto it = toplevel_variables.find(ENTRY_POINT);
    CHECK(it != toplevel_variables.end(), "No entry point found");
    auto var_expr = it->second;
    PrintF("%s\n", to_string(var_expr.SND->to_stringtree(), false));
    // Call the entry point function, will be called with unit arg.
    auto compiled_main_function = compile_function(var_expr.SND, {&bst::EXPR_EMPTY_TUPLE});
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
