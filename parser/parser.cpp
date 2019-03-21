#include <string>
#include <vector>

#include "absl/strings/str_format.h"

#include "util/log.h"

#include "command_line.h"
#include "consts.h"
#include "run_parser.h"

namespace forrest {

using absl::PrintF;
using std::string;
using std::vector;

static const char* const USAGE_TEXT =
    R"~~~~(%1$s: parse forrest-AST text file
Usage: %1$s --help
       %1$s <input-files>
)~~~~";

int parser_main(int argc, const char* argv[])
{
    log_program_name = PROGRAM_NAME;
    auto cl = parse_command_line(argc, argv);
    int result;
    if (cl.help) {
        PrintF(USAGE_TEXT, PROGRAM_NAME);
        result = EXIT_SUCCESS;
    } else if (cl.files.empty()) {
        fprintf(stderr, "No input files.\n");
        result = EXIT_FAILURE;
    } else
        result = run_parser(cl);
    return result;
}
}  // namespace forrest

int main(int argc, const char* argv[])
{
    try {
        return forrest::parser_main(argc, argv);
    } catch (std::exception& e) {
        fprintf(stderr, "Aborting, exception: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "Aborting, unknown exception\n");
    }
    return EXIT_FAILURE;
}
