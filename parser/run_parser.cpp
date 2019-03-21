#include "run_parser.h"

#include "absl/strings/str_format.h"
#include "ul/usual.h"
#include "util/filereader.h"

#include "command_line.h"
#include "errors.h"

namespace forrest {

using std::move;
using namespace ul;
using absl::StrFormat;

struct AstBuilder
{
    FileReader& fr;
    string error;
    AstBuilder(FileReader& fr) : fr(fr) {}

    void unexpected_char(Utf8Char c, int line, int col)
    {
        error = StrFormat("Unexpected char %s in file %s:%d:%d.", to_descriptive_string(c),
                          fr.filename, line, col);
    }
    bool read_exprs() {}
    bool read_expr()
    {
        // Skip whitespace
        for (;;) {
            fr.skip_whitespace();
            auto m_nc = fr.peek_char();
            if (!m_nc)
                break;
            auto nc = *m_nc;
            if (nc == '(') {  // Start Exprs
                if (!read_exprs())
                    return false;
            } else if (nc == '"') {  // Start AsciiStr/Str
            } else if (nc == '-' || nc == '+' || nc == '.' || isdigit(nc.xs[0])) {  // Start number
            } else if (nc == '{') {   // Start function application.
            } else if (nc == '\'') {  // Start rune
            } else if (nc == 't') {   // Start "true"
            } else if (nc == 'f') {   // Start "false"
            } else {
                // Error
                unexpected_char(nc, fr.line(), fr.col());
                return false;
            }
        }
    }
};

bool parse_file(const string& filename)
{
    auto lr = FileReader::new_(filename.c_str());
    if (is_left(lr)) {
        report_error(left(lr));
        return false;
    }
    auto fr = move(right(lr));
    AstBuilder ab(fr);
    if (!ab.read_expr())
        return false;

    return true;
}

int run_parser(const CommandLineOptions& o)
{
    bool ok = true;
    for (auto& f : o.files)
        if (!parse_file(f))
            ok = false;
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace forrest
