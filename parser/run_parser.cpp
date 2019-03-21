#include "run_parser.h"
#include "command_line.h"

namespace forrest {

class FileReader
{
public:
    static FileReader create(const char* s)
    {
        FILE* f = fopen(s, "rt");
        if (f == nullptr) {
        }
    }
};

int run_parser(const CommandLineOptions& o)
{
    for (const auto& f : o.files) {
        auto create_filereader = FileReader::create(f);
    }
    return 0;
}

}  // namespace forrest
