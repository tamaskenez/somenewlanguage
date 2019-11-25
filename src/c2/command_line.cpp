#include "command_line.h"

#include "ul/string.h"
#include "ul/ul.h"

namespace forrest {

using namespace ul;

maybe<CommandLineOptions> parse_command_line(int argc, const char* argv[])
{
    CommandLineOptions cl;
    FOR (i, 1, < argc) {
        auto a = argv[i];
        if (startswith(a, "--")) {
            a += 2;
            if (startswith(a, "help")) {
                cl.help = true;
            } else if (startswith(a, "cpp-out")) {
                if (++i == argc) {
                    fprintf(stderr, "Missing path for --cpp-out");
                    return {};
                } else {
                    cl.cpp_out = argv[i];
                }
            } else {
                fprintf(stderr, "invalid option: '%s'", argv[i]);
                return {};
            }
        } else {
            cl.files.emplace_back(a);
        }
    }
    return cl;
}
}  // namespace forrest
