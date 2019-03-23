#pragma once

#include <string>
#include <vector>

namespace forrest {

using std::string;
using std::vector;

struct CommandLineOptions
{
    bool help = false;
    vector<string> files;
};

CommandLineOptions parse_command_line(int argc, const char* argv[]);

}  // namespace forrest
