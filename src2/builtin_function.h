#pragma once

#include "common.h"
#include "term_forward.h"

namespace snl {
struct Store;
struct Context;

enum class BuiltinFunction
{
    Cast,
    Project,
    Cimport
};

struct InnerFunctionSignature
{
    unordered_set<term::Variable const*>
        forall_variables;  // A parameter is comptime if listed here.
    vector<snl::Parameter> parameters;
};
struct InnerFunctionDefinition
{
    using EvaluateTermFunction = std::function<optional<TermPtr>(Store& store, Context& context)>;
    using InferTypeOfTermFunction =
        std::function<optional<TermPtr>(Store& store, Context& context)>;

    string name;  // for debugging
    InnerFunctionSignature signature;
    EvaluateTermFunction evaluate_term_function;
    InferTypeOfTermFunction infer_type_of_term_function;
};

using BuiltinFunctionMap = unordered_map<BuiltinFunction, int>;
using InnerFunctionMap = unordered_map<int, InnerFunctionDefinition>;

unordered_map<BuiltinFunction, InnerFunctionDefinition> MakeBuiltinFunctions();
}  // namespace snl
