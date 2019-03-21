#include "run_parser.h"
#include "command_line.h"

#include "util/filereader.h"

namespace forrest {

void parse_file(const string& fname)
{
    auto ef = FileReader::new_(fname);
}

int run_parser(const CommandLineOptions& o)
{
    for (const auto& f : o.files) {
        parse_file(f);
    }
    return 0;
}

}  // namespace forrest
