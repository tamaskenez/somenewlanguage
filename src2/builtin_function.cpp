#include "builtin_function.h"

#include "astops.h"
#include "context.h"
#include "store.h"

namespace snl {

unordered_map<BuiltinFunction, InnerFunctionDefinition> MakeBuiltinFunctions(Store& store)
{
    using Tag = term::Tag;
    unordered_map<BuiltinFunction, InnerFunctionDefinition> m;

    // cast: forall A. comptime target_type::type_of_types source_value::A -> target_type
    auto cast_target_type_variable = store.MakeNewVariable(true);
    auto cast_source_type_variable = store.MakeNewVariable(true);
    vector<Parameter> cast_parameters = {
        Parameter(cast_target_type_variable, store.type_of_types),
        Parameter(store.MakeNewVariable(false), cast_source_type_variable)};
    InnerFunctionSignature cast_signature{
        unordered_set<term::Variable const*>(
            {cast_target_type_variable, cast_source_type_variable}),
        move(cast_parameters)};

    m[BuiltinFunction::Cast] = InnerFunctionDefinition{
        "cast", cast_signature,
        // Evaluate
        [signature = cast_signature](Store& store, Context& context) -> optional<TermPtr> {
            assert(signature.parameters.size() == 2);
            VAL_FROM_OPT_ELSE_RETURN(target_type, context.LookUp(signature.parameters[0].variable),
                                     nullopt);
            VAL_FROM_OPT_ELSE_RETURN(target_type_type, InferTypeOfTerm(store, context, target_type),
                                     nullopt);
            ASSERT_ELSE(target_type_type == store.type_of_types, return nullopt;);
            VAL_FROM_OPT_ELSE_RETURN(source_value, context.LookUp(signature.parameters[1].variable),
                                     nullopt);
            VAL_FROM_OPT_ELSE_RETURN(source_type, InferTypeOfTerm(store, context, source_value),
                                     nullopt);
            if (source_type == target_type) {
                return source_value;
            }
            UNREACHABLE;  // TODO for now, no casting.
            return nullopt;
        },
        // Infer type
        [signature = cast_signature](Store& store, Context& context) -> optional<TermPtr> {
            assert(signature.parameters.size() == 2);
            VAL_FROM_OPT_ELSE_RETURN(target_type, context.LookUp(signature.parameters[0].variable),
                                     nullopt);
            VAL_FROM_OPT_ELSE_RETURN(target_type_type, InferTypeOfTerm(store, context, target_type),
                                     nullopt);
            ASSERT_ELSE(target_type_type == store.type_of_types, return nullopt;);
            return target_type;
        }};

    // project: forall A. field_selector::StringLiteral subject::A
    auto project_subject_type_variable = store.MakeNewVariable(true);
    vector<Parameter> project_parameters = {
        Parameter(store.MakeNewVariable(true), store.string_literal_type),
        Parameter(store.MakeNewVariable(false), project_subject_type_variable)};
    InnerFunctionSignature project_signature{
        unordered_set<term::Variable const*>({project_subject_type_variable}),
        move(project_parameters)};

    m[BuiltinFunction::Project] = InnerFunctionDefinition{
        "project", project_signature,
        // Evaluate
        [signature = project_signature](Store& store, Context& context) -> optional<TermPtr> {
            ASSERT_ELSE(signature.parameters.size() == 2, return nullopt;);
            VAL_FROM_OPT_ELSE_RETURN(field_selector_term,
                                     context.LookUp(signature.parameters[0].variable), nullopt);
            using Tag = term::Tag;
            assert(field_selector_term->tag == Tag::StringLiteral);
            auto* field_selector = term_cast<term::StringLiteral>(field_selector_term);

            VAL_FROM_OPT_ELSE_RETURN(subject, context.LookUp(signature.parameters[1].variable),
                                     nullopt);
            ASSERT_ELSE(subject->tag == Tag::ProductValue, return nullopt;);
            auto* product_value = term_cast<term::ProductValue>(subject);
            auto it = product_value->values.find(field_selector->value);
            if (it == product_value->values.end()) {
                return nullopt;
            }
            return it->second;
        },
        // Infer type
        [](Store& store, Context& context) -> optional<TermPtr> {
            // ToBeInferred
            return store.MakeCanonical(term::SimpleTypeTerm(term::SimpleType::TypeToBeInferred));
        }};

    // cimport: source_code::StringLiteral -> E
    vector<Parameter> cimport_parameters = {
        Parameter(store.MakeNewVariable(true), store.string_literal_type)};
    InnerFunctionSignature cimport_signature{unordered_set<term::Variable const*>(),
                                             move(cimport_parameters)};

    m[BuiltinFunction::Cimport] = InnerFunctionDefinition{
        "cimport", cimport_signature,
        // Evaluate
        [signature = cimport_signature](Store& store, Context& context) -> optional<TermPtr> {
            assert(signature.parameters.size() == 1);
            VAL_FROM_OPT_ELSE_RETURN(cSourceCodeTerm,
                                     context.LookUp(signature.parameters[0].variable), nullopt);
            if (cSourceCodeTerm->tag != Tag::StringLiteral) {
                return nullopt;
            }
            auto& cSourceCode = term_cast<term::StringLiteral>(cSourceCodeTerm)->value;
            // TODO create a ProductType, ProductValue from the declaration after compiling
            // `cSourceCode`.
            unordered_map<string, TermPtr> fields;
            unordered_map<string, TermPtr> values;
            if (cSourceCode == "#include <cstdio>") {
                // int printf( const char *restrict format, ... );
                // StringLiteral -> Number
                InnerFunctionSignature printf_signature{
                    unordered_set<term::Variable const*>(),
                    vector<snl::Parameter>(
                        {Parameter(store.MakeNewVariable(true), store.string_literal_type)})};
                auto ifd = InnerFunctionDefinition{
                    "stdio.printf", printf_signature,
                    // Evaluate
                    [signature = printf_signature](Store& store,
                                                   Context& context) -> optional<TermPtr> {
                        ASSERT_ELSE(signature.parameters.size() == 1, return nullopt;);
                        VAL_FROM_OPT_ELSE_RETURN(format_string_term,
                                                 context.LookUp(signature.parameters[0].variable),
                                                 nullopt);
                        ASSERT_ELSE(InferTypeOfTerm(store, context, format_string_term) ==
                                        store.string_literal_type,
                                    return nullopt;);
                        auto format_string = term_cast<term::StringLiteral>(format_string_term);
                        auto result = printf("%s", format_string->value.c_str());
                        return new term::NumericLiteral(Number(result));
                    },
                    // Infer Type
                    [](Store& store, Context& context) -> optional<TermPtr> {
                        return store.numeric_literal_type;
                    }};
                auto if_id = store.AddInnerFunctionDefinition(move(ifd));
                fields["printf"] = new term::CppTerm(if_id);
            }
            auto product_type = store.MakeCanonical(term::ProductType(move(fields)));
            auto product_value =
                store.MakeCanonical(term::ProductValue(product_type, move(values)));
            return product_value;
        },
        // Infer type
        [](Store& store, Context& context) -> optional<TermPtr> {
            return store.MakeCanonical(term::SimpleTypeTerm(term::SimpleType::TypeToBeInferred));
        }};
    return m;
}
}  // namespace snl
