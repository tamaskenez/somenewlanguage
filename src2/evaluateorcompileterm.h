#pragma once

#include "common.h"
#include "term_forward.h"

namespace snl {
struct Context;
struct Store;
optional<TermPtr> EvaluateOrCompileTermSimpleAndSame(bool eval,
                                                     Store& store,
                                                     const Context& context,
                                                     TermPtr term);
}  // namespace snl
