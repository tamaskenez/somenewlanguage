#include "indented_lines.h"

namespace snl {

string FormatIndentedLines(const IndentedLines& indentedLines, bool emptyLineAfter)
{
    constexpr int kIndentationUnit = 4;
    int indentation = 0;
    string result;
    if (indentedLines.empty()) {
        return result;
    }
    for (auto& line : indentedLines) {
        switch_variant(
            line,
            [&indentation](ChangeIndentation changeIndentation) {
                switch (changeIndentation) {
                    case ChangeIndentation::Indent:
                        ++indentation;
                        break;
                    case ChangeIndentation::Dedent:
                        if (--indentation < 0) {
                            assert(false);
                            indentation = 0;
                        }
                        break;
                }
            },
            [indentation, &result](const string& s) {
                int spaces = indentation * kIndentationUnit;
                result.reserve(result.size() + spaces + s.size() + 1);
                result.append(spaces, ' ');
                result.append(s);
                result.push_back('\n');
            });
    }
    if (emptyLineAfter) {
        result.push_back('\n');
    }
    return result;
}

string FormatBlocksOfIndentedLines(const vector<IndentedLines>& blocks, bool emptyLineAfter)
{
    string result;
    for (int i = 0; i < blocks.size(); ++i) {
        result += FormatIndentedLines(blocks[i], emptyLineAfter || i + 1 < blocks.size());
    }
    return result;
}
}  // namespace snl