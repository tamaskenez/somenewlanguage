#include "program_type.h"

namespace snl {

namespace pt {
Store::Store()
    : bottom(MakeCanonical(BuiltIn{BuiltIn::Type::Bottom})),
      top(MakeCanonical(BuiltIn{BuiltIn::Type::Top})),
      unit(MakeCanonical(BuiltIn{BuiltIn::Type::Unit}))
{}

bool Store::IsCanonical(TypePtr x) const
{
    return canonical_types.count(*x) > 0;
}

TypePtr Store::MakeCanonicalCommon(Type&& t)
{
    auto tw = TypeWrapper{move(t)};
    auto it = canonical_types.find(tw);
    if (it == canonical_types.end()) {
        bool b;
        tie(it, b) = canonical_types.emplace(move(tw));
        assert(b);
    }
    return &*it;
}

TypePtr Store::MakeCanonical(BuiltIn&& t)
{
    return MakeCanonicalCommon(move(t));
}

TypePtr Store::MakeCanonical(Function&& t)
{
    assert(IsCanonical(t.domain));
    assert(IsCanonical(t.codomain));
    return MakeCanonicalCommon(move(t));
}

}  // namespace pt
}  // namespace snl
