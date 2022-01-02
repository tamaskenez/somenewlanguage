#include "term.h"
#include "freevariablesofterm.h"

namespace snl {
namespace term {

optional<Abstraction> Abstraction::MakeAbstraction(
    Store& store,
    unordered_set<Variable const*>&& forall_variables,
    vector<BoundVariable>&& bound_variables,
    vector<Parameter>&& parameters,
    TermPtr body)
{
    ASSERT_ELSE(!parameters.empty(), return nullopt;);
    auto bound_variables_so_far = *GetFreeVariables(store, body);
    bound_variables_so_far.erase(BE(forall_variables));
    for (auto bv : bound_variables) {
        bound_variables_so_far.erase(bv.variable);
    }
    for (auto par : parameters) {
        bound_variables_so_far.erase(par.variable);
    }

    auto unbound_forall_variables_so_far = forall_variables;

    // bound_variables_so_far contains variables coming from the context.
    for (auto bv : bound_variables) {
        // All free variables used in the RHS of the bound variable must be known.
        ASSERT_ELSE(is_subset_of(*GetFreeVariables(store, bv.value), bound_variables_so_far),
                    return nullopt;);
        // The bv itself must not be known.
        ASSERT_ELSE(bound_variables_so_far.insert(bv.variable).second, return nullopt;);
        unbound_forall_variables_so_far.erase(bv.variable);
    }

    for (auto par : parameters) {
        // All free variables used in the expected type must be either known or a yet unbound forall
        // variable.
        auto* fvs = GetFreeVariables(store, par.expected_type);
        for (auto fv : *fvs) {
            if (bound_variables_so_far.count(fv) == 0) {
                ASSERT_ELSE(unbound_forall_variables_so_far.count(fv) > 0, return nullopt;);
                bound_variables_so_far.insert(fv);
                unbound_forall_variables_so_far.erase(fv);
            }
        }
        // The parameter itself must not be known.
        ASSERT_ELSE(bound_variables_so_far.insert(par.variable).second, return nullopt;);
        unbound_forall_variables_so_far.erase(par.variable);  // It's there if it's comptime.
    }

    ASSERT_ELSE(unbound_forall_variables_so_far.empty(), return nullopt;);

    return Abstraction(move(forall_variables), move(bound_variables), move(parameters), body);
}

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

/*
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
        case Tag::ForAll:
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
*/

std::size_t TermHash::operator()(TermPtr t) const noexcept
{
    auto h = hash_value(t->tag);
    using namespace term;
    switch (t->tag) {
#define MAKE_U(TAG) auto& u = static_cast<const TAG&>(*t)
#define HC(X) hash_combine(h, X)
        case Tag::Abstraction: {
            MAKE_U(Abstraction);
            hash_range(h, BE(u.forall_variables));
            hash_range(h, BE(u.bound_variables));
            hash_range(h, BE(u.parameters));
            HC(u.body);
        } break;
        case Tag::LetIns: {
            MAKE_U(LetIns);
            hash_range(h, BE(u.bound_variables));
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
        case Tag::CppTerm: {
            MAKE_U(CppTerm);
            HC(u.id);
        } break;
        case Tag::StringLiteral: {
            MAKE_U(StringLiteral);
            HC(u.value);
        } break;
        case Tag::NumericLiteral: {
            MAKE_U(NumericLiteral);
            HC(u.value);
        } break;
        case Tag::UnitLikeValue: {
            MAKE_U(UnitLikeValue);
            HC(u.type);
        } break;
        case Tag::DeferredValue: {
            MAKE_U(DeferredValue);
            HC(u.type);
            HC(u.availability);
        } break;
        case Tag::ProductValue: {
            MAKE_U(ProductValue);
            HC(u.type);
            hash_range(h, BE(u.values));
        } break;
        case Tag::SimpleTypeTerm: {
            MAKE_U(SimpleTypeTerm);
            HC(u.simple_type);
        } break;
        case Tag::NamedType: {
            MAKE_U(NamedType);
            HC(u.name);
            HC(u.type_constructor);
        }
        case Tag::FunctionType: {
            MAKE_U(FunctionType);
            hash_range(h, BE(u.parameter_types));
            HC(u.return_type);
        } break;
        case Tag::TypeOfAbstraction: {
            MAKE_U(TypeOfAbstraction);
            HC(u.abstraction);
        } break;
        case Tag::ProductType: {
            MAKE_U(ProductType);
            hash_range(h, BE(u.members));
        } break;
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
            return u.body == v.body && u.forall_variables == v.forall_variables &&
                   u.parameters == v.parameters && u.bound_variables == v.bound_variables;
        }
        case Tag::LetIns: {
            MAKE_UV(LetIns);
            return u.body == v.body && u.bound_variables == v.bound_variables;
        }
        case Tag::Application: {
            MAKE_UV(Application);
            return u.function == v.function && u.arguments == v.arguments;
        }
        case Tag::CppTerm: {
            MAKE_UV(CppTerm);
            return u.id == v.id;
        }
        case Tag::Variable: {
            return false;  // Variables are always unique.
        }
        case Tag::StringLiteral: {
            MAKE_UV(StringLiteral);
            return u.value == v.value;
        }
        case Tag::NumericLiteral: {
            MAKE_UV(NumericLiteral);
            return u.value == v.value;
        }
        case Tag::SimpleTypeTerm: {
            MAKE_UV(SimpleTypeTerm);
            return u.simple_type == v.simple_type;
        }
        case Tag::NamedType: {
            MAKE_UV(NamedType);
            return u.name == v.name && u.type_constructor == v.type_constructor;
        }
        case Tag::FunctionType: {
            MAKE_UV(FunctionType);
            return u.return_type == v.return_type && u.parameter_types == v.parameter_types;
        }
        case Tag::TypeOfAbstraction: {
            MAKE_UV(TypeOfAbstraction);
            return u.abstraction == v.abstraction;
        }
        case Tag::ProductType: {
            MAKE_UV(ProductType);
            return u.members == v.members;
        }
        case Tag::UnitLikeValue: {
            MAKE_UV(UnitLikeValue);
            return u.type == v.type;
        }
        case Tag::DeferredValue: {
            MAKE_UV(DeferredValue);
            return u.type == v.type && u.availability == v.availability;
        }
        case Tag::ProductValue: {
            MAKE_UV(ProductValue);
            return u.type == v.type && u.values == v.values;
        }
#undef MAKE_UV
    }
}

}  // namespace snl
