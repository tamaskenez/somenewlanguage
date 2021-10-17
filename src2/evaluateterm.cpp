#include "astops.h"

#include "store.h"

namespace snl {
optional<TermPtr> EvaluateTerm(Store& store, const Context& context, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction:
            <#code #> break;
        case Tag::ForAll: {
            auto* tc = term_cast<term::ForAll>(term);
            auto new_term = EvaluateTerm(store, context, tc->term);
            if (!new_term) {
                return nullopt;
            }
            return store.MakeCanonical(term::ForAll(make_copy(tc->variables), *new_term));
        }
        case Tag::Application:
            <#code #> break;
        case Tag::Projection: {
            auto* projection = term_cast<term::Projection>(term);
            auto evaluated_domain = EvaluateTerm(store, context, projection->domain);
            if (!evaluated_domain) {
                return nullopt;
            }
            auto m_domain_type = InferTypeOfTerm(store, context, *evaluated_domain);
            if (!m_domain_type) {
                return nullopt;
            }
            auto domain_type = *m_domain_type;
            if (domain_type->tag != Tag::ProductType) {
                return nullopt;
            }
            auto product_type = term_cast<term::ProductType>(domain_type);
            auto member_index = product_type->FindMemberIndex(projection->codomain);
            if (!member_index) {
                return nullopt;
            }
            ASSERT_ELSE((*evaluated_domain)->tag == Tag::ProductValue, return nullopt;);
            auto product_value = term_cast<term::ProductValue>(*evaluated_domain);
            return product_value->values[*member_index];
        }
        case term::Tag::Cast: {
            auto* cast = term_cast<term::Cast>(term);
            auto new_subject = EvaluateTerm(store, context, cast->subject);
            if (!new_subject) {
                return nullopt;
            }
            auto new_target_type = EvaluateTerm(store, context, cast->target_type);
            if (!new_target_type) {
                return nullopt;
            }
            auto subject_type = InferTypeOfTerm(store, context, *new_subject);
            if (!subject_type) {
                return nullopt;
            }
            if (new_target_type == subject_type) {
                return *new_subject;
            }
            assert(false);  // TODO have no idea how to cast.
            return nullopt;
        }
        case Tag::UnitLikeValue: {
            auto* unit_like_value = term_cast<term::UnitLikeValue>(term);
            auto type = InferTypeOfTerm(store, context, term);
            if (!type) {
                return nullopt;
            }
            return store.MakeCanonical(term::UnitLikeValue(*type));
        }
        case Tag::DeferredValue:
            assert(false);  // I'm not sure if we ever allowed to evaluate this term.
            return nullopt;
        case Tag::ProductValue: {
            auto* pv = term_cast<term::ProductValue>(term);
            auto type = InferTypeOfTerm(store, context, term);
            if (!type) {
                return nullopt;
            }
            vector<TermPtr> new_values;
            for (auto v : pv->values) {
                auto new_value = EvaluateTerm(store, context, v);
                if (!new_value) {
                    return nullopt;
                }
                new_values.push_back(*new_value);
            }
            return store.MakeCanonical(term::ProductValue(*type, move(new_values)));
        }
        case Tag::FunctionType: {
            auto* tc = term_cast<term::FunctionType>(term);
            vector<TermPtr> new_parameter_types;
            for (auto pt : tc->parameter_types) {
                auto e = EvaluateTerm(store, context, pt);
                if (!e) {
                    return nullopt;
                }
                new_parameter_types.push_back(*e);
            }
            auto new_result_type = EvaluateTerm(store, context, tc->result_type);
            if (!new_result_type) {
                return nullopt;
            }
            return store.MakeCanonical(
                term::FunctionType(move(new_parameter_types), *new_result_type));
        }
        case Tag::ProductType: {
            auto* product_type = term_cast<term::ProductType>(term);
            vector<term::TaggedType> new_members;
            for (auto m : product_type->members) {
                auto new_type = EvaluateTerm(store, context, m.type);
                if (!new_type) {
                    return nullopt;
                }
                new_members.push_back(term::TaggedType{m.tag, *new_type});
            }
            return store.MakeCanonical(term::ProductType(move(new_members)));
        }
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
        case Tag::Variable:
            return term;
    }
}
}  // namespace snl
