#include <string>
#include <unordered_map>
#include <vector>

#include "absl/strings/str_format.h"
#include "ul/check.h"

#include "util/arena.h"
#include "util/filereader.h"
#include "util/log.h"

#include "ast.h"
#include "ast_builder.h"
#include "builtinnames.h"
#include "command_line.h"
#include "consts.h"
#include "cppgen.h"
#include "shell.h"

namespace forrest {

using absl::PrintF;
using std::make_unique;
using std::move;
using std::string;
using std::vector;

using ul::either;
using ul::is_left;
using ul::left;
using ul::right;

static const char* const USAGE_TEXT =
    R"~~~~(%1$s: parse forrest-AST text file
Usage: %1$s --help
       %1$s <input-files> [--cpp-out <filename>]
)~~~~";

// Add parsed data to ast.
maybe<vector<Node*>> parse_fast_file_add_to_ast(const string& filename, Arena& storage)
{
    auto lr = FileReader::new_(filename);
    if (is_left(lr)) {
        report_error(left(lr));
        return {};
    }
    // Call AstBuilder with new FileReader.
    return AstBuilder::parse_filereader_into_ast(right(lr), storage);
}

int run_fc_with_parsed_command_line(const CommandLineOptions& o)
{
    if (o.files.empty()) {
        fprintf(stderr, "No input files.\n");
        return EXIT_FAILURE;
    }

    bool ok = true;
    Arena storage;
    vector<Node*> top_level_exprs;
    for (auto& f : o.files) {
        auto m_exprs = parse_fast_file_add_to_ast(f, storage);
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
    printf("Top level expressions.\n");
    for (auto x : top_level_exprs)
        dump(x);

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

    return EXIT_SUCCESS;
}

int fc_main(int argc, const char** argv)
{
    g_log.program_name = PROGRAM_NAME;
    BuiltinNames::init_g();

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
        return forrest::fc_main(argc, argv);
    } catch (std::exception& e) {
        fprintf(stderr, "Aborting, exception: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "Aborting, unknown exception\n");
    }
    return EXIT_FAILURE;
}
