#include "builtin_abstraction.h"
#include "astops.h"
#include "context.h"
#include "store.h"

namespace snl {

struct BuiltinAbstractions
{
    BuiltInAbstraction cast, project;
};

BuiltinAbstractions GetBuiltinAbstractions()
{
    BuiltinAbstractions result;
    result.cast = BuiltInAbstraction{
        // Evaluate
        [](Store& store, Context& context, const vector<TermPtr>& arguments) -> optional<TermPtr> {
            ASSERT_ELSE(arguments.size() == 2, return nullopt;);
            auto target_type = arguments[0];
            VAL_FROM_OPT_ELSE_RETURN(target_type_type, InferTypeOfTerm(store, context, target_type),
                                     nullopt);
            ASSERT_ELSE(target_type_type == store.type_of_types, return nullopt;);
            auto source_value = arguments[1];
            VAL_FROM_OPT_ELSE_RETURN(source_type, InferTypeOfTerm(store, context, source_value),
                                     nullopt);
            if (source_type == target_type) {
                return source_value;
            }
            UNREACHABLE;  // TODO for now, no casting.
            return nullopt;
        },
        // Infer type
        [](Store& store, Context& context, const vector<TermPtr>& arguments) -> optional<TermPtr> {
            switch (arguments.size()) {
                case 0: {
                    // ForAll targetType SourceType: targetType::TypeOfTypes ->
                    // sourceValue::SourceType -> TargetType
                    auto target_type = store.MakeNewVariable(true);
                    auto source_type = store.MakeNewVariable(true);
                    unordered_set<term::Variable const*> forall_variables(
                        {target_type, source_type});
                    vector<TypeAndAvailability> parameter_types(
                        {TypeAndAvailability(store.type_of_types, target_type),
                         TypeAndAvailability(source_type, nullopt)});
                    TermPtr result_type = target_type;
                    return store.MakeCanonical(term::FunctionType(
                        move(forall_variables), move(parameter_types), result_type));
                }
                case 1: {
                    // ForAll SourceType: sourceValue::SourceType -> TargetType
                    auto target_type = arguments[0];
                    VAL_FROM_OPT_ELSE_RETURN(target_type_type,
                                             InferTypeOfTerm(store, context, target_type), nullopt);
                    ASSERT_ELSE(target_type_type == store.type_of_types, return nullopt;);
                    auto source_type = store.MakeNewVariable(true);
                    unordered_set<term::Variable const*> forall_variables({source_type});
                    vector<TypeAndAvailability> parameter_types(
                        {TypeAndAvailability(source_type, nullopt)});
                    TermPtr result_type = target_type;
                    return store.MakeCanonical(term::FunctionType(
                        move(forall_variables), move(parameter_types), result_type));
                }
                case 2: {
                    // TargetType
                    auto target_type = arguments[0];
                    VAL_FROM_OPT_ELSE_RETURN(target_type_type,
                                             InferTypeOfTerm(store, context, target_type), nullopt);
                    ASSERT_ELSE(target_type_type == store.type_of_types, return nullopt;);
                    return target_type;
                }
                default:
                    UNREACHABLE;
                    return nullopt;
            }
        }};
    result.project = BuiltInAbstraction{
        // Evaluate
        [](Store& store, Context& context, const vector<TermPtr>& arguments) -> optional<TermPtr> {
            ASSERT_ELSE(arguments.size() == 2, return nullopt;);
            auto field_selector_term = arguments[0];
            using Tag = term::Tag;
            ASSERT_ELSE(field_selector_term->tag != Tag::StringLiteral, return nullopt;);
            auto* field_selector = term_cast<term::StringLiteral>(field_selector_term);

            auto subject = arguments[1];
            ASSERT_ELSE(subject->tag != Tag::ProductValue, return nullopt;);
            auto* product_value = term_cast<term::ProductValue>(subject);
            auto it = product_value->values.find(field_selector->value);
            if (it == product_value->values.end()) {
                return nullopt;
            }
            return it->second;
        },
        // Infer type
        [](Store& store, Context& context, const vector<TermPtr>& arguments) -> optional<TermPtr> {
            switch (arguments.size()) {
                case 0: {
                    // ForAll fieldSelector SubjectType:
                    // fieldSelector::StringLiteral -> subject::SubjectType -> ToBeInferred
                    auto field_selector = store.MakeNewVariable(true);
                    auto subject_type = store.MakeNewVariable(true);
                    unordered_set<term::Variable const*> forall_variables(
                        {field_selector, subject_type});
                    vector<TypeAndAvailability> parameter_types(
                        {TypeAndAvailability(store.string_literal_type, field_selector),
                         TypeAndAvailability(subject_type, nullopt)});
                    TermPtr result_type = store.MakeCanonical(
                        term::SimpleTypeTerm(term::SimpleType::TypeToBeInferred));
                    return store.MakeCanonical(term::FunctionType(
                        move(forall_variables), move(parameter_types), result_type));
                }
                case 1: {
                    // ForAll SubjectType:
                    // subject::SubjectType -> ToBeInferred
                    auto subject_type = store.MakeNewVariable(true);
                    unordered_set<term::Variable const*> forall_variables({subject_type});
                    vector<TypeAndAvailability> parameter_types(
                        {TypeAndAvailability(subject_type, nullopt)});
                    TermPtr result_type = store.MakeCanonical(
                        term::SimpleTypeTerm(term::SimpleType::TypeToBeInferred));
                    return store.MakeCanonical(term::FunctionType(
                        move(forall_variables), move(parameter_types), result_type));
                }
                case 2: {
                    // ToBeInferred
                    return store.MakeCanonical(
                        term::SimpleTypeTerm(term::SimpleType::TypeToBeInferred));
                }
                default:
                    UNREACHABLE;
                    return nullopt;
            }
        }};
    return result;
}
}  // namespace snl
