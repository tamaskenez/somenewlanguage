#include "term.h"

namespace snl {
namespace term {

/*
TermPtr Abstraction::TypeFromParametersAndResult(Store& store,
                                                 const vector<Parameter>& parameters,
                                                 TermPtr result_type)
{
    vector<TermPtr> terms;
    for (auto& p : parameters) {
        if (auto expected_type = p.expected_type) {
            terms.push_back(*expected_type);
        } else {
            terms.push_back(store.MakeNewVariable());
        }
    }
    terms.push_back(result_type);
    return store.MakeCanonical(FunctionType(move(terms)));
}

TermPtr Abstraction::ResultType() const
{
    if (parameters.empty()) {
        return type;
    } else {
        auto function_type = term_cast<FunctionType>(type);
        assert(function_type->operand_types.size() == parameters.size() + 1);
        return function_type->operand_types.back();
    }
}
*/

}  // namespace term
/*
void DepthFirstTraversal(TermPtr p, std::function<bool(TermPtr)>& f)
{
    if (f(p)) {
        return;
    }
    switch (p->tag) {
        case Tag::Abstraction: {
            auto q = term_cast<Abstraction>(p);
            for (auto& par : q->parameters) {
                if (par.type_annotation.has_value()) {
                    DepthFirstTraversal(*par.type_annotation, f);
                }
            }
            for (auto& var : q->bound_variables) {
                DepthFirstTraversal(var.value, f);
            }
            DepthFirstTraversal(q->body, f);
        } break;
        case Tag::Application: {
            auto q = term_cast<Application>(p);
            DepthFirstTraversal(q->function, f);
            for (auto a : q->arguments) {
                DepthFirstTraversal(a, f);
            }
        } break;
        case Tag::Variable:
            break;
        case Tag::Projection: {
            auto q = term_cast<Projection>(p);
            DepthFirstTraversal(q->domain, f);
        } break;
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
            break;
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
        case Tag::InferredType:
            break;
        case Tag::FunctionType: {
            auto q = term_cast<FunctionType>(p);
            for (auto t : q->terms) {
                DepthFirstTraversal(t, f);
            }
        }
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
            break;
    }
}
*/

bool IsTypeInNormalForm(TermPtr p)
{
    using Tag = term::Tag;
    switch (p->tag) {
        case Tag::Abstraction:
        // Even an application with comptime arguments and comptime type result is not in normal
        // form. These applications must be executed and they produce a new type. More precisely,
        // either an existing type (e.g. Int) or a new type (e.g. List Int) which is registered as a
        // single, new term and associated with the application that produces it (APP List Int).
        case Tag::Application:
        case Tag::Variable:
        case Tag::Projection:
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::UnitLikeValue:
        case Tag::RuntimeValue:
        case Tag::ComptimeValue:
        case Tag::ProductValue:
            return false;
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
            return true;
        case Tag::FunctionType: {
            auto q = term_cast<term::FunctionType>(p);
            for (auto t : q->operand_types) {
                if (!IsTypeInNormalForm(t)) {
                    return false;
                }
            }
            return true;
        }
        case Tag::ProductType:
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
            return true;
    }
}

std::size_t TermHash::operator()(TermPtr t) const noexcept
{
    auto h = hash_value(t->tag);
    using namespace term;
    switch (t->tag) {
#define MAKE_U(TAG) auto& u = static_cast<const TAG&>(*t)
#define HC(X) hash_combine(h, X)
        case Tag::Abstraction: {
            MAKE_U(Abstraction);
            hash_range(h, BE(u.implicit_type_parameters));
            hash_range(h, BE(u.bound_variables));
            hash_range(h, BE(u.parameters));
            HC(u.body);
        } break;
        case Tag::Application: {
            MAKE_U(Application);
            HC(u.function);
            hash_range(h, BE(u.arguments));
        } break;
        case Tag::Variable: {
            HC(t);  // Each variable instance is unique.
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
        case Tag::UnitLikeValue:
        case Tag::RuntimeValue:
        case Tag::ComptimeValue: {
            auto* value_term = static_cast<ValueTerm const*>(t);
            assert(value_term);
            HC(value_term->type);
        } break;
        case Tag::ProductValue: {
            MAKE_U(ProductValue);
            HC(u.type);
            hash_range(h, BE(u.values));
        } break;
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
            break;
        case Tag::FunctionType: {
            MAKE_U(FunctionType);
            hash_range(h, BE(u.operand_types));
        } break;
        case Tag::ProductType: {
            MAKE_U(ProductType);
            hash_range(h, BE(u.members));
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
    if (x->tag != y->tag) {
        return false;
    }
    using namespace term;
    switch (x->tag) {
#define MAKE_UV(TAG)                       \
    auto& u = static_cast<const TAG&>(*x); \
    auto& v = static_cast<const TAG&>(*y)
        case Tag::Abstraction: {
            MAKE_UV(Abstraction);
            return u.body == v.body && u.parameters == v.parameters &&
                   u.bound_variables == v.bound_variables &&
                   u.implicit_type_parameters == v.implicit_type_parameters;
        } break;
        case Tag::Application: {
            MAKE_UV(Application);
            return u.function == v.function && u.arguments == v.arguments;
        }
        case Tag::Variable: {
            return false;  // Variables are always unique.
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
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
            return true;
        case Tag::FunctionType: {
            MAKE_UV(FunctionType);
            return u.operand_types == v.operand_types;
        }
        case Tag::ProductType: {
            MAKE_UV(ProductType);
            return u.members == v.members;
        }
        case Tag::UnitLikeValue:
        case Tag::RuntimeValue:
        case Tag::ComptimeValue: {
            auto* u = static_cast<ValueTerm const*>(x);
            assert(u);
            auto* v = static_cast<ValueTerm const*>(y);
            assert(v);
            return u->type == v->type;
        }
        case Tag::ProductValue: {
            MAKE_UV(ProductValue);
            return u.type == v.type && u.values == v.values;
        }
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
            return true;
#undef MAKE_UV
    }
}

Store::Store()
    : type_of_types(MakeCanonical(term::TypeOfTypes())),
      bottom_type(MakeCanonical(term::BottomType())),
      unit_type(MakeCanonical(term::UnitType())),
      top_type(MakeCanonical(term::TopType())),
      string_literal_type(MakeCanonical(term::StringLiteralType())),
      numeric_literal_type(MakeCanonical(term::NumericLiteralType()))
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
            CASE(Application)
            CASE(Variable)
            CASE(Projection)

            CASE(StringLiteral)
            CASE(NumericLiteral)
            CASE(UnitLikeValue)
            CASE(RuntimeValue)
            CASE(ComptimeValue)
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
        CASE(Abstraction, move(t.implicit_type_parameters), move(t.bound_variables),
             move(t.parameters), t.body)
        CASE(Application, t.function, move(t.arguments))
        CASE(Variable, move(t.name))
        CASE(Projection, t.domain, move(t.codomain))

        CASE(StringLiteral, move(t.value))
        CASE(NumericLiteral, move(t.value))

        CASE0(TypeOfTypes)
        CASE0(BottomType)
        CASE0(UnitType)
        CASE0(TopType)
        CASE(FunctionType, move(t.operand_types))
        CASE(ProductType, move(t.members))
        CASE(UnitLikeValue, t.type)
        CASE(RuntimeValue, t.type)
        CASE(ComptimeValue, t.type)
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

term::Variable const* Store::MakeNewVariable(string&& name)
{
    auto p = new term::Variable(name.empty() ? fmt::format("GV#{}", next_generated_variable_id++)
                                             : move(name));
    auto itb = canonical_terms.insert(p);
    assert(itb.second);
    return p;
}

string const Store::s_ignored_name;  // Empty string.

FreeVariables const* Store::MakeCanonical(FreeVariables&& fv)
{
    return &*canonicalized_free_variables.insert(move(fv)).first;
}

}  // namespace snl
