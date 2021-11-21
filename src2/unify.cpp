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
        case Tag::InnerAbstraction:
        case Tag::LetIns:
            // There should be no Abstraction or LetIns in an evaluated expected type.
            UNREACHABLE;
            return nullopt;
        case Tag::Application:
            // This must be an Application with ForAll variables to be unified, that's why it's not
            // yet evaluated.
            {
                auto* application = term_cast<term::Application>(expected_type);
                // We need to find the same function
                term::Abstraction const* abstraction;
                // `abstraction` must be unified with arg_type which is supposed to be a
                // named type which has a type constructor function.
                // If the function matches, unify the arguments
                UNREACHABLE;  // TODO implement.
                return nullopt;
            }
        case Tag::Variable: {
            auto* variable = term_cast<term::Variable>(expected_type);
            // There can't be any non-forall variables, everything must be evaluated.
            ASSERT_ELSE(forall_variables.count(variable) > 0, return nullopt;);
            // It can't be in the inner context because all variables of the inner context are
            // replaced with their values before entering this switch.
            ASSERT_ELSE(inner_context.variables.count(variable) == 0, return nullopt;);
            inner_context.Bind(variable, arg_type);
            return tuple<>();
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
            auto it = product_value->values.begin();
            auto jt = arg_product_value->values.begin();
            for (;; ++it, ++jt) {
                if (it == product_value->values.end()) {
                    if (jt == product_value->values.end()) {
                        break;
                    } else {
                        return nullopt;
                    }
                }
                if (it->first != jt->first) {
                    return nullopt;
                }
                auto value = it->second;
                auto arg_value = jt->second;
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
        case Tag::NamedType: {
            if (expected_type == arg_type) {
                return tuple<>();
            }
            return nullopt;
        }
        case Tag::FunctionType: {
            // TODO subtyping: expected function type's parameters must be subtypes of arg function
            // type's parameters. expected function type's result must be supertype of arg function
            // type's parameters.
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
                if (arg_par.comptime_parameter.has_value() && !par.comptime_parameter.has_value()) {
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
            // TODO subtyping
            if (arg_type->tag != Tag::ProductType) {
                return nullopt;
            }
            auto* product_type = term_cast<term::ProductType>(expected_type);
            auto* arg_product_type = term_cast<term::ProductType>(arg_type);
            if (product_type == arg_product_type) {
                return tuple<>();
            }
            // Decide if they're compatible.
            // TODO: implement unnamed ProductType comptability check.
            // Certain unnamed product types might be compatible with each other, if the fields
            // match.
            return nullopt;
        }
    }
    return nullopt;
}

}  // namespace snl
