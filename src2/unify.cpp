#include "unify.h"
#include "astops.h"
#include "freevariablesofterm.h"

namespace snl {
optional<UnifyResult> Unify(Store& store,
                            const Context& context,
                            TermPtr pattern,
                            TermPtr concrete,
                            const unordered_set<term::Variable const*>& variables_to_unify)
{
    assert(false);
    return nullopt;
}

bool HasIntersection(const unordered_map<term::Variable const*, TermPtr>& new_bound_variables,
                     const unordered_set<term::Variable const*> free_variables)
{
    if (new_bound_variables.size() < free_variables.size()) {
        for (auto [variable, term] : new_bound_variables) {
            if (free_variables.count(variable) > 0) {
                return true;
            }
        }
    } else {
        for (auto variable : free_variables) {
            if (new_bound_variables.count(variable) > 0) {
                return true;
            }
        }
    }
    return false;
}

optional<tuple<>> UnifyExpectedTypeToArgType(
    Store& store,
    Context& inner_context,
    const unordered_set<term::Variable const*>& forall_variables,
    TermPtr expected_type,
    TermPtr arg_type)
{
    using Tag = term::Tag;
    // Re-evaluate expected_type if it contains a free variable that has already been unified.
    auto* fvs = GetFreeVariables(store, expected_type);
    if (HasIntersection(inner_context.variables, *fvs)) {
        VAL_FROM_OPT_ELSE_RETURN(evaluated_expected_type,
                                 EvaluateTerm(store, inner_context, expected_type), nullopt);
        expected_type = evaluated_expected_type;
    }
    switch (expected_type->tag) {
        case Tag::Abstraction:
            // There should be no Abstraction in an evaluated expected type.
            UNREACHABLE;
            return nullopt;
        case Tag::ForAll:
            // Since ForAll will only be used for Abstractions, this is not allowed,either.
            UNREACHABLE;
            return nullopt;
        case Tag::Application:
            // This must be an Application with ForAll variables to be unified, that's why it's not
            // yet evaluated.
            {
                auto* application = term_cast<term::Application>(expected_type);
                // We need to find the same function
                term::ForAll const* for_all;
                term::Abstraction const* abstraction;
                //(for_all, abstraction) must be unified with arg_type which is supposed to be a
                // type which has a generator function.
                // If the function matches, unify the arguments
                UNREACHABLE;  // TODO implement.
                return nullopt;
            }
        case Tag::Variable: {
            auto* variable = term_cast<term::Variable>(expected_type);
            ASSERT_ELSE(forall_variables.count(variable) > 0,
                        return nullopt;);  // There can't be any other variables, everything must be
                                           // evaluated.
            ASSERT_ELSE(
                inner_context.variables.count(variable) == 0,
                return nullopt;);  // Unified variables are also need to be evaluated for this term.
            inner_context.Bind(variable, arg_type);
            return tuple<>();
        }
        case Tag::Projection: {
            if (arg_type->tag != Tag::Projection) {
                return nullopt;
            }
            auto* projection = term_cast<term::Projection>(expected_type);
            auto* arg_projection = term_cast<term::Projection>(arg_type);
            if (projection->codomain != arg_projection->codomain) {
                return nullopt;
            }
            return UnifyExpectedTypeToArgType(store, inner_context, forall_variables, projection,
                                              arg_projection);
        }
        case Tag::Cast: {
            // Cast is not unifyable because a Cast in the arg_type is reduced away by the
            // evaluation.
            return nullopt;
        }
        case Tag::StringLiteral: {
            if (arg_type->tag != Tag::StringLiteral) {
                return nullopt;
            }
            auto* string_literal = term_cast<term::StringLiteral>(expected_type);
            auto* arg_string_literal = term_cast<term::StringLiteral>(arg_type);
            if (string_literal->value == arg_string_literal->value) {
                return tuple<>();
            }
            return nullopt;
        }
        case Tag::NumericLiteral: {
            if (arg_type->tag != Tag::NumericLiteral) {
                return nullopt;
            }
            auto* numeric_literal = term_cast<term::NumericLiteral>(expected_type);
            auto* arg_numeric_literal = term_cast<term::NumericLiteral>(arg_type);
            switch (Compare(numeric_literal->value, arg_numeric_literal->value)) {
                case NumberCompareResultOrNaN::Less:
                case NumberCompareResultOrNaN::Greater:
                case NumberCompareResultOrNaN::OneOfThemIsNaN:
                    return nullopt;
                case NumberCompareResultOrNaN::Equal:
                case NumberCompareResultOrNaN::BothAreNaNs:
                    return tuple<>();
            }
        }
        case Tag::UnitLikeValue: {
            if (arg_type->tag != Tag::UnitLikeValue) {
                return nullopt;
            }
            auto* unit_like_value = term_cast<term::UnitLikeValue>(expected_type);
            auto* arg_unit_like_value = term_cast<term::UnitLikeValue>(arg_type);
            return UnifyExpectedTypeToArgType(store, inner_context, forall_variables,
                                              unit_like_value->type, arg_unit_like_value->type);
        }
        case Tag::DeferredValue:
            // Neither runtime nor comptime deferred value is allowed in an evaluated expression.
            UNREACHABLE;
            return nullopt;
        case Tag::ProductValue: {
            if (arg_type->tag != Tag::ProductValue) {
                return nullopt;
            }
            auto* product_value = term_cast<term::ProductValue>(expected_type);
            auto* arg_product_value = term_cast<term::ProductValue>(arg_type);
            if (product_value->values.size() != arg_product_value->values.size()) {
                return nullopt;
            }
            auto type_result =
                UnifyExpectedTypeToArgType(store, inner_context, forall_variables,
                                           product_value->type, arg_product_value->type);
            if (!type_result) {
                return nullopt;
            }
            ASSERT_ELSE(product_value->values.size() == arg_product_value->values.size(),
                        return nullopt;);
            for (int i = 0; i < product_value->values.size(); ++i) {
                auto value = product_value->values[i];
                auto arg_value = arg_product_value->values[i];
                auto value_result = UnifyExpectedTypeToArgType(store, inner_context,
                                                               forall_variables, value, arg_value);
                if (!value_result) {
                    return nullopt;
                }
            }
            return tuple<>();
        }
        case Tag::SimpleTypeTerm: {
            if (arg_type->tag != Tag::SimpleTypeTerm) {
                return nullopt;
            }
            auto* simple_type_term = term_cast<term::SimpleTypeTerm>(expected_type);
            auto* arg_simple_type_term = term_cast<term::SimpleTypeTerm>(arg_type);
            if (simple_type_term->simple_type == arg_simple_type_term->simple_type) {
                return tuple<>();
            }
            return nullopt;
        }
        case Tag::FunctionType: {
            // TODO subtyping
            // TODO instead of UnresolvedAbstraction we need ToBeInferred type

            if (arg_type->tag != Tag::FunctionType) {
                return nullopt;
            }
            auto* function_type = term_cast<term::FunctionType>(expected_type);
            auto* arg_function_type = term_cast<term::FunctionType>(arg_type);
            if (function_type->parameter_types.size() !=
                arg_function_type->parameter_types.size()) {
                return nullopt;
            }
            for (int i = 0; i < function_type->parameter_types.size(); ++i) {
                auto par = function_type->parameter_types[i];
                auto arg_par = arg_function_type->parameter_types[i];
                // If the abstraction is waiting for a lambda with a runtime parameter but the
                // lambda needs comptime parameter, it's an error.
                if (arg_par.comptime && !par.comptime) {
                    return nullopt;
                }
                auto par_result = UnifyExpectedTypeToArgType(store, inner_context, forall_variables,
                                                             par.type, arg_par.type);
                if (!par_result) {
                    return nullopt;
                }
            }
            auto result_result = UnifyExpectedTypeToArgType(store, inner_context, forall_variables,
                                                            function_type->result_type,
                                                            arg_function_type->result_type);
            if (!result_result) {
                return nullopt;
            }
            return tuple<>();
        }
        case Tag::ProductType: {
            // TODO subtyping, named types
            if (arg_type->tag != Tag::ProductType) {
                return nullopt;
            }
            auto* product_type = term_cast<term::ProductType>(expected_type);
            auto* arg_product_type = term_cast<term::ProductType>(arg_type);
            if (product_type->members.size() != arg_product_type->members.size()) {
                return nullopt;
            }
            for (int i = 0; i < product_type->members.size(); ++i) {
                auto member = product_type->members[i];
                auto arg_member = arg_product_type->members[i];
                if (member.tag != arg_member.tag) {
                    return nullopt;
                }
                auto member_result = UnifyExpectedTypeToArgType(
                    store, inner_context, forall_variables, member.type, arg_member.type);
                if (!member_result) {
                    return nullopt;
                }
            }
            return tuple<>();
        }
    }
    return nullopt;
}

}  // namespace snl
