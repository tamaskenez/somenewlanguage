#pragma once

#include "common.h"

namespace snl {

enum class ChangeIndentation
{
    Indent,
    Dedent
};
using IndentedLine = variant<ChangeIndentation, string>;
using IndentedLines = vector<IndentedLine>;  // Dedent back to zero is implicit at the end.

string FormatIndentedLines(const IndentedLines& indentedLines, bool emptyLineAfter);
string FormatBlocksOfIndentedLines(const vector<IndentedLines>& blocks, bool emptyLineAfter);

}  // namespace snl