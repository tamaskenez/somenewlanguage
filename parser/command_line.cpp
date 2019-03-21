#include "command_line.h"

#include "ul/string.h"
#include "ul/ul.h"

namespace forrest {

using namespace ul;

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
