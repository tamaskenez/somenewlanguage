#include "command_line.h"

namespace forrest {

CommandLineOptions parse_command_line(int argc, const char* argv[])
{
    CommandLineOptions cl;
    FOR (i, 1, < argc) {
        auto a = argv[i];
        if (startswith(a, "--")) {
            a += 2;
            if (startswith(a, "help"))
                cl.help = true;
            else
                fprintf(stderr, "invalid option: '%s'", argv[i]);
        } else {
            cl.files.emplace_back(a);
        }
    }
    return cl;
}
}  // namespace forrest
