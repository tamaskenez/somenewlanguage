#include <string>
#include <unordered_map>
#include <vector>

#include "absl/strings/str_format.h"
#include "ul/check.h"

#include "util/filereader.h"
#include "util/log.h"

#include "ast.h"
#include "ast_builder.h"
#include "command_line.h"
#include "consts.h"
#include "cppgen.h"

namespace forrest {

using absl::PrintF;
using std::make_unique;
using std::move;
using std::string;
using std::vector;

static const char* const USAGE_TEXT =
    R"~~~~(%1$s: parse forrest-AST text file
Usage: %1$s --help
       %1$s <input-files> [--cpp-out <filename>]
)~~~~";

// Add parsed data to ast.
bool parse_fast_file_add_to_ast(const string& filename, Ast& ast)
{
    auto lr = FileReader::new_(filename);
    if (is_left(lr)) {
        report_error(left(lr));
        return false;
    }
    // Call AstBuilder with new FileReader.
    return AstBuilder::parse_filereader_into_ast(right(lr), ast);
}

int run_fc_with_parsed_command_line(const CommandLineOptions& o)
{
    if (o.files.empty()) {
        fprintf(stderr, "No input files.\n");
        return EXIT_FAILURE;
    }

    bool ok = true;
    Ast ast;
    for (auto& f : o.files) {
        if (parse_fast_file_add_to_ast(f, ast))
            absl::PrintF("Compiled %s\n", f);
        else
            ok = false;
    }
    if (ok) {
        dump(ast);
        if (!o.cpp_out.empty()) {
            ok = cppgen(ast, o);
        }
    }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

int fc_main(int argc, const char** argv)
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
        return forrest::fc_main(argc, argv);
    } catch (std::exception& e) {
        fprintf(stderr, "Aborting, exception: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "Aborting, unknown exception\n");
    }
    return EXIT_FAILURE;
}
