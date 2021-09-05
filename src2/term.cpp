#include "term.h"

namespace snl {
namespace term {

StringLiteral::StringLiteral(Store& store, string&& value)
    : Term(Tag::Variable, store.string_literal_type), value(move(value))
{}

NumericLiteral::NumericLiteral(Store& store, string&& value)
    : Term(Tag::NumericLiteral, store.string_literal_type), value(move(value))
{}

SequenceYieldLast::SequenceYieldLast(Store& store, vector<TermPtr>&& terms)
    : Term(Tag::SequenceYieldLast, terms.empty() ? store.bottom_type : terms.back()->type),
      terms(move(terms))
{
    assert(!terms.empty());
}

std::size_t TermHash::operator()(TermPtr t) const noexcept
{
    auto h = hash_value(t->tag);
    if (t->type != t) {            // Prevent recursive call for TypeOfTypes.
        hash_combine(h, t->type);  // Plain pointer-hash.
    }
    switch (t->tag) {
#define MAKE_U(TAG) auto& u = static_cast<const TAG&>(*t)
#define HC(X) hash_combine(h, X)
        case Tag::Abstraction: {
            MAKE_U(Abstraction);
            hash_range(h, BE(u.parameters));
            HC(u.body);
        } break;
        case Tag::Application: {
            MAKE_U(Application);
            HC(u.function);
            hash_range(h, BE(u.arguments));
        } break;
        case Tag::Variable: {
            MAKE_U(Variable);
            HC(u.name);
        } break;
        case Tag::Projection: {
            MAKE_U(Projection);
            HC(u.domain);
            HC(u.codomain);
        } break;
        case Tag::StringLiteral: {
            MAKE_U(StringLiteral);
            HC(u.value);
        } break;
        case Tag::NumericLiteral: {
            MAKE_U(NumericLiteral);
            HC(u.value);
        } break;
        case Tag::LetIn: {
            MAKE_U(LetIn);
            HC(u.variable_name);
            HC(u.initializer);
            HC(u.body);
        } break;
        case Tag::SequenceYieldLast: {
            MAKE_U(SequenceYieldLast);
            hash_range(h, BE(u.terms));
        } break;
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
            break;
        case Tag::InferredType: {
            MAKE_U(InferredType);
            HC(u.id);
        } break;
        case Tag::FunctionType: {
            MAKE_U(FunctionType);
            hash_range(h, BE(u.terms));
        } break;
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
            break;
#undef MAKE_U
#undef HC
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
        case Tag::Projection: {
            MAKE_UV(Projection);
            return u.domain == v.domain && u.codomain == v.codomain;
        }
        case Tag::StringLiteral: {
            MAKE_UV(StringLiteral);
            return u.value == v.value;
        }
        case Tag::NumericLiteral: {
            MAKE_UV(NumericLiteral);
            return u.value == v.value;
        }
        case Tag::LetIn: {
            MAKE_UV(LetIn);
            return u.variable_name == v.variable_name && u.initializer == v.initializer &&
                   u.body == v.body;
        }
        case Tag::SequenceYieldLast: {
            MAKE_UV(SequenceYieldLast);
            return u.terms == v.terms;
        }
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
            return true;
        case Tag::InferredType: {
            MAKE_UV(InferredType);
            return u.id == v.id;
        }
        case Tag::FunctionType: {
            MAKE_UV(FunctionType);
            return u.terms == v.terms;
        }
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
            return true;
#undef MAKE_UV
    }
}

Store::Store()
    : type_of_types(&s_type_of_types_canonical_instance),
      canonical_terms({type_of_types}),
      bottom_type(MakeCanonical(BottomType())),
      unit_type(MakeCanonical(UnitType())),
      top_type(MakeCanonical(TopType())),
      string_literal_type(MakeCanonical(StringLiteralType())),
      numeric_literal_type(MakeCanonical(NumericLiteralType()))
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
            CASE(Projection)

            CASE(StringLiteral)
            CASE(NumericLiteral)

            CASE(LetIn)
            CASE(SequenceYieldLast)

            CASE(BottomType)
            CASE(UnitType)
            CASE(TopType)
            CASE(InferredType)
            CASE(FunctionType)

            CASE(StringLiteralType)
            CASE(NumericLiteralType)

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
        CASE(Variable, t.type, move(t.name))
        CASE(Projection, t.type, t.domain, move(t.codomain))

        CASE(StringLiteral, *this, move(t.value))
        CASE(NumericLiteral, *this, move(t.value))

        CASE(LetIn, t.type, move(t.variable_name), t.initializer, t.body)
        CASE(SequenceYieldLast, *this, move(t.terms))

        CASE(BottomType)
        CASE(UnitType)
        CASE(TopType)
        CASE(InferredType, t.id)
        CASE(FunctionType, move(t.terms))

        CASE(StringLiteralType)
        CASE(NumericLiteralType)
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

TermPtr Store::MakeInferredTypeTerm()
{
    auto p = new InferredType(next_inferred_type_id++);
    auto itb = canonical_terms.insert(p);
    assert(itb.second);
    return p;
}

TypeOfTypes Store::s_type_of_types_canonical_instance;

TypeTerm::TypeTerm(Tag tag) : Term(tag, &Store::s_type_of_types_canonical_instance) {}

}  // namespace term
}  // namespace snl
