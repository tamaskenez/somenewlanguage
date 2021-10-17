#include "freevariablesofterm.h"
#include "store.h"

namespace snl {

FreeVariables FreeVariables::CopyToTypeLevel() const
{
    FreeVariables result;
    for (auto [k, v] : variables) {
        result.variables.insert(make_pair(k, VariableUsage::FlowsIntoType));
    }
    return result;
}
bool FreeVariables::EraseVariable(term::Variable const* v)
{
    return variables.erase(v) == 1;
}
void FreeVariables::InsertWithKeepingStronger(const FreeVariables& fv)
{
    for (auto [k, v] : fv.variables) {
        switch (v) {
            case VariableUsage::FlowsIntoType:
                variables[k] = VariableUsage::FlowsIntoType;
                break;
            case VariableUsage::FlowsIntoValue:
                // Insert only if it's not there.
                variables.insert(make_pair(k, VariableUsage::FlowsIntoValue));
                break;
        }
    }
}
bool FreeVariables::DoesFlowIntoType(term::Variable const* v) const
{
    auto it = variables.find(v);
    return it != variables.end() && it->second == VariableUsage::FlowsIntoType;
}

FreeVariables GetFreeVariablesCore(Store& store, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction: {
            auto abstraction = term_cast<term::Abstraction>(term);
            auto fv = *GetFreeVariables(store, abstraction->body);
            std::unordered_set<term::Variable const*> vars_used_by_par_types;
            for (auto& p : abstraction->parameters) {
                auto fvs_of_expected_type =
                    keys_as_vector(GetFreeVariables(store, p.expected_type)->variables);
                vars_used_by_par_types.insert(BE(fvs_of_expected_type));
                fv.InsertWithFlowingToType(fvs_of_expected_type);
            }
            for (auto& p : abstraction->parameters) {
                if (fv.DoesFlowIntoType(p.variable)) {
                    store.about_variables[p.variable].flows_into_type = true;
                }
                fv.EraseVariable(p.variable);
            }
            for (auto it = abstraction->bound_variables.rbegin();
                 it != abstraction->bound_variables.rend(); ++it) {
                bool flows_into_types = fv.DoesFlowIntoType(it->variable);
                if (flows_into_types) {
                    store.about_variables[it->variable].flows_into_type = true;
                }
                fv.EraseVariable(it->variable);
                auto* fv_of_value = GetFreeVariables(store, it->value);
                if (flows_into_types) {
                    auto keys = keys_as_vector(fv_of_value->variables);
                    fv.InsertWithFlowingToType(keys);
                } else {
                    fv.InsertWithKeepingStronger(*fv_of_value);
                }
            }
            return fv;
        }
        case Tag::ForAll: {
            auto* forall = term_cast<term::ForAll>(term);
            auto fvs = GetFreeVariablesCore(store, forall->term);
            for (auto v : forall->variables) {
                bool erased = fvs.EraseVariable(v);
                assert(erased);
                (void)erased;
            }
            return fvs;
        }
        case Tag::Application: {
            auto application = term_cast<term::Application>(term);
            auto fv = *GetFreeVariables(store, application->function);
            for (auto& a : application->arguments) {
                fv.InsertWithKeepingStronger(*GetFreeVariables(store, a));
            }
            return fv;
        }
        case Tag::Variable: {
            FreeVariables fv;
            fv.variables[term_cast<term::Variable>(term)] =
                FreeVariables::VariableUsage::FlowsIntoValue;
            return fv;
        }
        case Tag::Projection: {
            auto projection = term_cast<term::Projection>(term);
            return *GetFreeVariables(store, projection->domain);
        }
        case Tag::Cast: {
            // TODO: Cast could be an internal function.
            // We should solve anyway that we can mark parameters comptime so their free variables
            // will be marked comptime, too.
            auto* cast = term_cast<term::Cast>(term);
            auto fvs = *GetFreeVariables(store, cast->subject);
            auto fv_target_type = GetFreeVariables(store, cast->target_type);
            fvs.InsertWithFlowingToType(keys_as_vector(fv_target_type->variables));
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
        case Tag::NumericLiteralType: {
            assert(false);  // This should be caught in GetFreeVariables().
            const static FreeVariables empty_fv;
            return empty_fv;
        }
        case Tag::DeferredValue: {
            auto* dv = term_cast<term::DeferredValue>(term);
            return GetFreeVariables(store, dv->type)->CopyToTypeLevel();
        }
        case Tag::ProductValue: {
            auto product_value = term_cast<term::ProductValue>(term);
            FreeVariables fv;
            for (auto& v : product_value->values) {
                fv.InsertWithKeepingStronger(*GetFreeVariables(store, v));
            }
            return fv;
        }
        case Tag::FunctionType: {
            auto function_type = term_cast<term::FunctionType>(term);
            FreeVariables fv;
            for (auto p : function_type->parameter_types) {
                fv.InsertWithKeepingStronger(*GetFreeVariables(store, p));
            }
            fv.InsertWithKeepingStronger(*GetFreeVariables(store, function_type->result_type));
            return fv;
        }
        case Tag::ProductType: {
            auto product_type = term_cast<term::ProductType>(term);
            FreeVariables fv;
            for (auto m : product_type->members) {
                fv.InsertWithKeepingStronger(*GetFreeVariables(store, m.type));
            }
            return fv;
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
