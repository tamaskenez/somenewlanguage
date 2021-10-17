#include "unify.h"

namespace snl {
optional<UnifyResult> Unify(Store& store,
                            const Context& context,
                            TermPtr pattern,
                            TermPtr concrete,
                            const unordered_set<term::Variable const*>& variables_to_unify)
{
    assert(false);
    return nullopt;
}
}  // namespace snl
