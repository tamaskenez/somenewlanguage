#include "evaluateorcompileterm.h"

#include "astops.h"
#include "context.h"
#include "store.h"

namespace snl {

optional<TermPtr> EvaluateOrCompileTerm(bool eval,
                                        Store& store,
                                        const Context& context,
                                        TermPtr term)
{
    return eval ? EvaluateTerm(store, context, term) : CompileTerm(store, context, term);
}

optional<TermPtr> EvaluateOrCompileTermSimpleAndSame(bool eval,
                                                     Store& store,
                                                     const Context& context,
                                                     TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case term::Tag::Abstraction:
        case term::Tag::Application:

            assert(false);
            return nullopt;

        case term::Tag::ForAll: {
            auto* tc = term_cast<term::ForAll>(term);
            VAL_FROM_OPT_ELSE_RETURN(
                new_term, EvaluateOrCompileTerm(eval, store, context, tc->term), nullopt);
            return store.MakeCanonical(term::ForAll(make_copy(tc->variables), new_term));
        }
        case term::Tag::Cast: {
            auto* cast = term_cast<term::Cast>(term);
            VAL_FROM_OPT_ELSE_RETURN(
                new_subject, EvaluateOrCompileTerm(eval, store, context, cast->subject), nullopt);
            VAL_FROM_OPT_ELSE_RETURN(new_target_type,
                                     EvaluateTerm(store, context, cast->target_type), nullopt);
            if (eval) {
                VAL_FROM_OPT_ELSE_RETURN(subject_type, InferTypeOfTerm(store, context, new_subject),
                                         nullopt);
                if (new_target_type == subject_type) {
                    return new_subject;
                }
                assert(false);  // TODO have no idea how to cast. We should call the built-in `cast`
                                // function.
                return nullopt;
            } else {
                return store.MakeCanonical(term::Cast(new_subject, new_target_type));
            }
        }
        case term::Tag::Projection: {
            auto* projection = term_cast<term::Projection>(term);
            VAL_FROM_OPT_ELSE_RETURN(
                new_domain, EvaluateOrCompileTerm(eval, store, context, projection->domain),
                nullopt);
            VAL_FROM_OPT_ELSE_RETURN(domain_type, InferTypeOfTerm(store, context, new_domain),
                                     nullopt);
            if (domain_type->tag != Tag::ProductType) {
                return nullopt;
            }
            auto product_type = term_cast<term::ProductType>(domain_type);
            VAL_FROM_OPT_ELSE_RETURN(member_index,
                                     product_type->FindMemberIndex(projection->codomain), nullopt);
            if (eval) {
                ASSERT_ELSE(new_domain->tag == Tag::ProductValue, return nullopt;);
                auto product_value = term_cast<term::ProductValue>(new_domain);
                return product_value->values[member_index];
            } else {
                return store.MakeCanonical(
                    term::Projection(new_domain, make_copy(projection->codomain)));
            }
        }
        case term::Tag::UnitLikeValue: {
            auto* unit_like_value = term_cast<term::UnitLikeValue>(term);
            VAL_FROM_OPT_ELSE_RETURN(type, InferTypeOfTerm(store, context, term), nullopt);
            return store.MakeCanonical(term::UnitLikeValue(type));
        }
        case term::Tag::DeferredValue:
            assert(false);  // I'm not sure if we ever allowed to evaluate this term.
            return nullopt;
        case term::Tag::ProductValue: {
            auto* pv = term_cast<term::ProductValue>(term);
            vector<TermPtr> new_values;
            for (auto v : pv->values) {
                VAL_FROM_OPT_ELSE_RETURN(new_value, EvaluateOrCompileTerm(eval, store, context, v),
                                         nullopt);
                new_values.push_back(new_value);
            }
            VAL_FROM_OPT_ELSE_RETURN(type, InferTypeOfTerm(store, context, term), nullopt);
            return store.MakeCanonical(term::ProductValue(type, move(new_values)));
        }
        case term::Tag::FunctionType: {
            auto function_type = term_cast<term::FunctionType>(term);
            vector<TermPtr> new_parameter_types;
            for (auto p : function_type->parameter_types) {
                VAL_FROM_OPT_ELSE_RETURN(new_parameter,
                                         EvaluateOrCompileTerm(eval, store, context, p), nullopt);
                new_parameter_types.push_back(new_parameter);
            }
            VAL_FROM_OPT_ELSE_RETURN(
                new_result_type,
                EvaluateOrCompileTerm(eval, store, context, function_type->result_type), nullopt);
            return store.MakeCanonical(
                term::FunctionType(move(new_parameter_types), new_result_type));
        }
        case term::Tag::ProductType: {
            auto* product_type = term_cast<term::ProductType>(term);
            vector<term::TaggedType> new_members;
            for (auto m : product_type->members) {
                VAL_FROM_OPT_ELSE_RETURN(
                    new_type, EvaluateOrCompileTerm(eval, store, context, m.type), nullopt);
                new_members.push_back(term::TaggedType{m.tag, new_type});
            }
            return store.MakeCanonical(term::ProductType(move(new_members)));
        }

        case term::Tag::Variable:
        case term::Tag::StringLiteral:
        case term::Tag::NumericLiteral:
        case term::Tag::TypeOfTypes:
        case term::Tag::UnitType:
        case term::Tag::BottomType:
        case term::Tag::TopType:
        case term::Tag::StringLiteralType:
        case term::Tag::NumericLiteralType:
            return term;
    }
}
}  // namespace snl
