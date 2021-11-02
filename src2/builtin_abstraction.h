#pragma once

#include "common.h"
#include "term.h"

namespace snl {
struct Store;

struct BuiltInAbstraction
{
    using EvaluateTermFunction = std::function<
        optional<TermPtr>(Store& store, Context& context, const vector<TermPtr>& arguments)>;
    using InferTypeOfTermFunction = std::function<
        optional<TermPtr>(Store& store, Context& context, const vector<TermPtr>& arguments)>;

    EvaluateTermFunction evaluate_term_function;
    InferTypeOfTermFunction infer_type_of_term_function;
};

// forall codomain, Domain. projection codomain::ct-string domain::Domain {}
// forall Domain.ct-string -> Domain -> ToBeInferredType

// forall SourceType, target_type.Cast subject::SourceType -> target_type::TypeOfTypes ->
// target_type
}  // namespace snl
