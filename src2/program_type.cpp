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

TypePtr Store::MakeCanonical(Type2&& t2)
{
    auto t1 = Type1{move(t2)};
    auto it = canonical_types.find(t1);
    if (it == canonical_types.end()) {
        bool b;
        tie(it, b) = canonical_types.emplace(move(t1));
        assert(b);
    }
    return &*it;
}

TypePtr Store::MakeCanonical(BuiltIn&& t)
{
    return MakeCanonical(move(t));
}

TypePtr Store::MakeCanonical(Function&& t)
{
    assert(IsCanonical(t.domain));
    assert(IsCanonical(t.codomain));
    return MakeCanonical(move(t));
}

}  // namespace pt
}  // namespace snl
