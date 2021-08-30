#include "program_type.h"

namespace snl {

namespace pt {
Store::Store()
    : bottom(MakeCanonical(BuiltIn{BuiltIn::Type::Bottom})),
      top(MakeCanonical(BuiltIn{BuiltIn::Type::Top})),
      unit(MakeCanonical(BuiltIn{BuiltIn::Type::Unit}))
{}

bool Store::IsCanonical(Type x) const
{
#define CASE(I) \
    [this](std::variant_alternative_t<I, Type> p) { return std::get<I>(type_maps).count(*p) > 0; }
    return switch_variant(x, CASE(0), CASE(1), CASE(2), CASE(3), CASE(4), CASE(5));
#undef CASE
}

template <size_t I>
Type MakeCanonical(Store& store, std::remove_pointer_t<std::variant_alternative_t<I, Type>>& t)
{
    auto& type_map = std::get<I>(store.type_maps);
    auto it = type_map.find(t);
    if (it == type_map.end()) {
        bool b;
        tie(it, b) = type_map.insert(t);
        assert(b);
    }
    return &*it;
}

Type Store::MakeCanonical(const BuiltIn& t)
{
    return pt::MakeCanonical<0>(*this, t);
}

Type Store::MakeCanonical(const Function& t)
{
    assert(IsCanonical(t.domain));
    assert(IsCanonical(t.codomain));
    return pt::MakeCanonical<5>(*this, t);
}

}  // namespace pt
}  // namespace snl
