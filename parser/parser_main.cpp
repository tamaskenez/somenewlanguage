#include <string>
#include <unordered_map>
#include <vector>

#include "absl/strings/str_format.h"
#include "ul/check.h"

#include "util/filereader.h"
#include "util/log.h"

#include "ast_builder.h"
#include "command_line.h"
#include "consts.h"

namespace forrest {

using absl::PrintF;
using std::make_unique;
using std::move;
using std::string;
using std::vector;

static const char* const USAGE_TEXT =
    R"~~~~(%1$s: parse forrest-AST text file
Usage: %1$s --help
       %1$s <input-files>
)~~~~";

bool parse_file(const string& filename)
{
    auto lr = FileReader::new_(filename.c_str());
    if (is_left(lr)) {
        report_error(left(lr));
        return false;
    }
    auto fr = move(right(lr));
    auto ab = AstBuilder::new_(fr);
    if (!ab->parse())
        return false;

    return true;
}

int run_parser(const CommandLineOptions& o)
{
    bool ok = true;
    for (auto& f : o.files) {
        if (!parse_file(f))
            ok = false;
        absl::PrintF("Compiled %s\n", f);
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

int parser_main(int argc, const char* argv[])
{
    g_log.program_name = PROGRAM_NAME;
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

struct Immovable
{
    Immovable(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
    Immovable& operator=(const Immovable&) = delete;
    Immovable& operator=(Immovable&&) = delete;
    Immovable(int a, int b) : a(a), b(b) {}
    int a, b;
};

void test_hash_map_pointer_stability()
{
    // No need to call, if it compiles, OK.
    StableHashMap<std::string, Immovable> hm;
    hm.try_emplace("one", 1, 2);
    hm.rehash(1000);
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
