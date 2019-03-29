#pragma once

#include <string>
#include <vector>

#include "util/maybe.h"

namespace forrest {

using std::string;
using std::vector;

struct CommandLineOptions
{
    bool help = false;
    vector<string> files;
    string cpp_out;
};

maybe<CommandLineOptions> parse_command_line(int argc, const char* argv[]);

}  // namespace forrest
