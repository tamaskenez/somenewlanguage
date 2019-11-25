#pragma once

#include "fmt/format.h"

#include "consts.h"

namespace forrest {

template <typename... Args>
void report_error(const string& se)
{
    fmt::print(stderr, "{}: error: {}.\n", PROGRAM_NAME, se);
}

}  // namespace forrest
