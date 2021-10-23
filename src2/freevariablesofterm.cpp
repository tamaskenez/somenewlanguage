#include "freevariablesofterm.h"
#include "store.h"

namespace snl {

FreeVariables GetFreeVariablesCore(Store& store, TermPtr term)
{
    using Tag = term::Tag;
    const static FreeVariables empty_fvs;
    switch (term->tag) {
        case Tag::Abstraction: {
            auto abstraction = term_cast<term::Abstraction>(term);
            auto fvs = *GetFreeVariables(store, abstraction->body);
            /*
            std::unordered_set<term::Variable const*> vars_used_by_par_types;
            for (auto& p : abstraction->parameters) {
                auto fvs_of_expected_type =
                    keys_as_vector(GetFreeVariables(store, p.expected_type)->variables);
                vars_used_by_par_types.insert(BE(fvs_of_expected_type));
                fv.InsertWithFlowingToType(fvs_of_expected_type);
            }*/
            for (auto& p : abstraction->parameters) {
                auto* fvs_expected_type = GetFreeVariables(store, p.expected_type);
                fvs.insert(BE(*fvs_expected_type));
            }
            for (auto& p : abstraction->parameters) {
                fvs.erase(p.variable);
            }
            for (auto it = abstraction->bound_variables.rbegin();
                 it != abstraction->bound_variables.rend(); ++it) {
                fvs.erase(it->variable);
                auto* fvs_of_value = GetFreeVariables(store, it->value);
                fvs.insert(BE(*fvs_of_value));
            }
            return fvs;
        }
        case Tag::ForAll: {
            auto* forall = term_cast<term::ForAll>(term);
            auto fvs = *GetFreeVariables(store, forall->term);
            for (auto v : forall->variables) {
                bool erased = fvs.erase(v);
                assert(erased);
                (void)erased;
            }
            return fvs;
        }
        case Tag::Application: {
            auto application = term_cast<term::Application>(term);
            auto fvs = *GetFreeVariables(store, application->function);
            for (auto& a : application->arguments) {
                auto* fvs_argument = GetFreeVariables(store, a);
                fvs.insert(BE(*fvs_argument));
            }
            return fvs;
        }
        case Tag::Variable:
            return FreeVariables({term_cast<term::Variable>(term)});
        case Tag::Projection:
            return *GetFreeVariables(store, term_cast<term::Projection>(term)->domain);
        case Tag::Cast: {
            auto* cast = term_cast<term::Cast>(term);
            auto fvs = *GetFreeVariables(store, cast->subject);
            auto* fvs_target_type = GetFreeVariables(store, cast->target_type);
            fvs.insert(BE(*fvs_target_type));
            return fvs;
        }
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::UnitLikeValue:
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
            assert(false);  // This should be caught in GetFreeVariables().
            return empty_fvs;
        case Tag::DeferredValue:
            assert(false);  // This should be put only into a Variable which is never dereferenced
                            // here.
            return empty_fvs;
        case Tag::ProductValue: {
            auto product_value = term_cast<term::ProductValue>(term);
            FreeVariables fvs;
            for (auto& v : product_value->values) {
                auto* fvs_value = GetFreeVariables(store, v);
                fvs.insert(BE(*fvs_value));
            }
            return fvs;
        }
        case Tag::FunctionType: {
            auto function_type = term_cast<term::FunctionType>(term);
            FreeVariables fvs;
            for (auto p : function_type->parameter_types) {
                auto* fvs_p = GetFreeVariables(store, p);
                fvs.insert(BE(*fvs_p));
            }
            auto* fvs_result_type = GetFreeVariables(store, function_type->result_type);
            fvs.insert(BE(*fvs_result_type));
            return fvs;
        }
        case Tag::ProductType: {
            auto product_type = term_cast<term::ProductType>(term);
            FreeVariables fvs;
            for (auto m : product_type->members) {
                auto* fvs_m = GetFreeVariables(store, m.type);
                fvs.insert(BE(*fvs_m));
            }
            return fvs;
        }
    }
}

FreeVariables const* GetFreeVariables(Store& store, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::UnitLikeValue:
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType: {
            const static FreeVariables empty_fv;
            return &empty_fv;
        }
        default:
            break;
    }
    auto it = store.free_variables_of_terms.find(term);
    if (it == store.free_variables_of_terms.end()) {
        it = store.free_variables_of_terms
                 .insert(make_pair(term, store.MakeCanonical(GetFreeVariablesCore(store, term))))
                 .first;
    }
    return it->second;
}

}  // namespace snl
