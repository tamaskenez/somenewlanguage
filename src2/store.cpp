#include "store.h"

namespace snl {
Store::Store()
    : type_of_types(MakeCanonical(term::TypeOfTypes())),
      bottom_type(MakeCanonical(term::BottomType())),
      unit_type(MakeCanonical(term::UnitType())),
      top_type(MakeCanonical(term::TopType())),
      string_literal_type(MakeCanonical(term::StringLiteralType())),
      numeric_literal_type(MakeCanonical(term::NumericLiteralType())),
      comptime_type_value(MakeCanonical(
          term::DeferredValue(type_of_types, term::DeferredValue::Availability::Comptime))),
      comptime_value_comptime_type(MakeCanonical(
          term::DeferredValue(comptime_type_value, term::DeferredValue::Availability::Comptime)))
{}

Store::~Store()
{
    for (auto p : canonical_terms) {
        using namespace term;
        switch (p->tag) {
#define CASE(X)                          \
    case Tag::X:                         \
        delete static_cast<X const*>(p); \
        break;
            CASE(Abstraction)
            CASE(ForAll)
            CASE(Application)
            CASE(Variable)
            CASE(Projection)
            CASE(Cast)

            CASE(StringLiteral)
            CASE(NumericLiteral)
            CASE(UnitLikeValue)
            CASE(DeferredValue)
            CASE(ProductValue)

            CASE(TypeOfTypes)

            CASE(BottomType)
            CASE(UnitType)
            CASE(TopType)
            CASE(FunctionType)
            CASE(ProductType)

            CASE(StringLiteralType)
            CASE(NumericLiteralType)

#undef CASE
        }
    }
}

TermPtr Store::MoveToHeap(Term&& term)
{
    using namespace term;
    switch (term.tag) {
#define CASE0(TAG) \
    case Tag::TAG: \
        return new TAG();
#define CASE(TAG, ...)                     \
    case Tag::TAG: {                       \
        auto& t = static_cast<TAG&>(term); \
        return new TAG(__VA_ARGS__);       \
    }
        CASE(Abstraction, move(t.bound_variables), move(t.parameters), t.body)
        CASE(ForAll, move(t.variables), t.term)
        CASE(Application, t.function, move(t.arguments))
        CASE(Variable, t.comptime, move(t.name))
        CASE(Projection, t.domain, move(t.codomain))
        CASE(Cast, t.subject, t.target_type)

        CASE(StringLiteral, move(t.value))
        CASE(NumericLiteral, move(t.value))

        CASE0(TypeOfTypes)
        CASE0(BottomType)
        CASE0(UnitType)
        CASE0(TopType)
        CASE(FunctionType, move(t.parameter_types), t.result_type)
        CASE(ProductType, move(t.members))
        CASE(UnitLikeValue, t.type)
        CASE(DeferredValue, t.type, t.role)
        CASE(ProductValue, term_cast<ProductType>(t.type), move(t.values))

        CASE0(StringLiteralType)
        CASE0(NumericLiteralType)
#undef CASE0
#undef CASE
    }
}

TermPtr Store::MakeCanonical(Term&& t)
{
    assert(t.tag != term::Tag::Variable);
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

term::Variable const* Store::MakeNewVariable(bool comptime, string&& name)
{
    auto p = new term::Variable(
        comptime, name.empty() ? fmt::format("GV#{}", next_generated_variable_id++) : move(name));
    auto itb = canonical_terms.insert(p);
    assert(itb.second);
    return p;
}

string const Store::s_ignored_name;  // Empty string.

FreeVariables const* Store::MakeCanonical(FreeVariables&& fv)
{
    return &*canonicalized_free_variables.insert(move(fv)).first;
}

optional<TermPtr> Store::GetOrInsertTypeOfTermInContext(
    TermWithBoundFreeVariables&& term_with_bound_free_variables,
    std::function<optional<TermPtr>()> make_type_fn)
{
    auto it = types_of_terms_in_context.find(term_with_bound_free_variables);
    if (it == types_of_terms_in_context.end()) {
        auto type = make_type_fn();
        if (!type) {
            return nullopt;
        }
        it =
            types_of_terms_in_context.insert(make_pair(move(term_with_bound_free_variables), *type))
                .first;
    }
    return it->second;
}

}  // namespace snl
