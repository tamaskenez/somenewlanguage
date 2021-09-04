#include "term.h"

namespace snl {
namespace term {

template <class C>
struct ContainerHash
{
    std::size_t operator()(const C& c) const noexcept
    {
        std::size_t h = sizeof(typename C::value_type);
        for (auto& x : c) {
            h = (h << 1) ^ std::hash<typename C::value_type>{}(x);
        }
        return h;
    }
};

template <class C>
std::size_t ContainerHashFunction(const C& c) noexcept
{
    return ContainerHash<C>{}(c);
}

std::size_t TermHash::operator()(TermPtr t) const noexcept
{
    std::size_t h = std::hash<Tag>{}(t->tag);
    if (t->type != t) {                                // Prevent recursive call for TypeOfTypes.
        h = (h << 1) ^ std::hash<TermPtr>{}(t->type);  // Plain pointer-hash.
    }
    switch (t->tag) {
#define MAKE_U(TAG) auto& u = static_cast<const TAG&>(*t)
        case Tag::Abstraction: {
            MAKE_U(Abstraction);
            h = (h << 1) ^ ContainerHashFunction(u.parameters);
            h = (h << 1) ^ std::hash<TermPtr>{}(u.body);
        } break;
        case Tag::Application: {
            MAKE_U(Application);
            h = (h << 1) ^ std::hash<TermPtr>{}(u.function);
            h = (h << 1) ^ ContainerHashFunction(u.arguments);
        } break;
        case Tag::Variable: {
            MAKE_U(Variable);
            h = (h << 1) ^ std::hash<string>{}(u.name);
        } break;
        case Tag::StringLiteral: {
            MAKE_U(StringLiteral);
            h = (h << 1) ^ std::hash<string>{}(u.value);
        } break;
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
            break;
        case Tag::StringLiteralType: {
            MAKE_U(StringLiteralType);
            h = (h << 1) ^ std::hash<int>{}(u.size);
        } break;
#undef MAKE_U
    }
    return h;
}

bool TermEqual::operator()(TermPtr x, TermPtr y) const noexcept
{
    if (x == y) {
        return true;
    }
    if (x->tag != y->tag || x->type != y->type) {
        return false;
    }
    switch (x->tag) {
#define MAKE_UV(TAG)                       \
    auto& u = static_cast<const TAG&>(*x); \
    auto& v = static_cast<const TAG&>(*y)
        case Tag::Abstraction: {
            MAKE_UV(Abstraction);
            return u.body == v.body && u.parameters == v.parameters;
        } break;
        case Tag::Application: {
            MAKE_UV(Application);
            return u.function == v.function && u.arguments == v.arguments;
        }
        case Tag::Variable: {
            MAKE_UV(Variable);
            return u.name == v.name;
        }
        case Tag::StringLiteral: {
            MAKE_UV(StringLiteral);
            return u.value == v.value;
        }
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
            return true;
        case Tag::StringLiteralType: {
            MAKE_UV(StringLiteralType);
            return u.size == v.size;
        } break;
#undef MAKE_UV
    }
}

Store::Store()
    : type_of_types(&s_type_of_types_canonical_instance),
      canonical_terms({type_of_types}),
      bottom_type(MakeCanonical(BottomType{})),
      unit_type(MakeCanonical(UnitType{})),
      top_type(MakeCanonical(TopType{}))
{}

Store::~Store()
{
    for (auto p : canonical_terms) {
        if (p == &s_type_of_types_canonical_instance) {
            continue;
        }
        switch (p->tag) {
#define CASE(X)                          \
    case Tag::X:                         \
        delete static_cast<X const*>(p); \
        break;
            CASE(Abstraction)
            CASE(Application)
            CASE(Variable)
            CASE(StringLiteral)

            CASE(BottomType)
            CASE(UnitType)
            CASE(TopType)
            CASE(StringLiteralType)

#undef CASE
            case Tag::TypeOfTypes:
                assert(false);
                break;
        }
    }
}

TermPtr Store::MoveToHeap(Term&& term)
{
    switch (term.tag) {
#define CASE(TAG, ...)                     \
    case Tag::TAG: {                       \
        auto& t = static_cast<TAG&>(term); \
        return new TAG(__VA_ARGS__);       \
    } break;
        CASE(Abstraction, t.type, move(t.parameters), t.body)
        CASE(Application, t.type, t.function, move(t.arguments))
        CASE(StringLiteral, *this, move(t.value))
        CASE(Variable, t.type, move(t.name))
        CASE(BottomType)
        CASE(UnitType)
        CASE(TopType)
        CASE(StringLiteralType, t.size)
#undef CASE
        case Tag::TypeOfTypes:
            assert(false);
            return new TypeOfTypes;
    }
}

TermPtr Store::MakeCanonical(Term&& t)
{
    auto it = canonical_terms.find(&t);
    if (it == canonical_terms.end()) {
        bool b;
        tie(it, b) = canonical_terms.emplace(MoveToHeap(move(t)));
        assert(b);
    }
    return *it;
}

bool Store::IsCanonical(TermPtr t) const
{
    return canonical_terms.count(t) > 0;
}

TypeOfTypes Store::s_type_of_types_canonical_instance;

TypeTerm::TypeTerm(Tag tag) : Term(tag, &Store::s_type_of_types_canonical_instance) {}

}  // namespace term
}  // namespace snl
