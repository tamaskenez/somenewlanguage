#pragma once

#include <string>

namespace forrest {

// TODO when this fails: add #defines to avoid redefining char8_t when it's supported in the
// language (C++20)
using char8_t = unsigned char;
using u8string = std::basic_string<char8_t>;

static_assert(
    sizeof(char8_t) == 1,
    "Think about this and probably replace char8_t with uint8_t avoiding excessive memory usage.");

}  // namespace forrest
