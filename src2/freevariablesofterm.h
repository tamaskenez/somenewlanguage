#pragma once

#include "term.h"

namespace snl {
using FreeVariables = unordered_set<term::Variable const*>;
FreeVariables const* GetFreeVariables(Store& store, TermPtr term);

}  // namespace snl
