#include "bst.h"
#include "text_ast.h"

namespace snl {
Bst::Bst()
{
    ts.Add(MakeUnionType("type", {"unit"}));
    ts.Add(MakeProductType("parameter", {{"name", "string"}, {"type", "type"}}));
    ts.Add(MakeVectorType("vector-parameter", "parameter"));
    ts.Add(MakeVectorType("vector-expression", "expression"));
    ts.Add(MakeProductType("lambda-abstraction",
                           {{"parameters", "vector-parameter"}, {"body", "expression"}}));
    ts.Add(MakeProductType(
        "let-expression",
        {{"variable-name", "string"}, {"initializer", "expression"}, {"body", "expression"}}));
    ts.Add(MakeProductType("function-application", {{"function-expression", "expression"},
                                                    {"arguments", "vector-expression"}}));
    ts.Add(MakeProductType("expression-sequence", {{"expressions", "vector-expression"}}));
    ts.Add(MakeProductType("variable", {{"name", "string"}}));
    ts.Add(MakeBuiltInType("number"));
    ts.Add(MakeProductType("tuple", {{"expressions", "vector-expression"}}));
    ts.Add(MakeUnionType("expression",
                         {"lambda-abstraction", "let-expression", "function-application",
                          "expression-sequence", "string", "variable", "tuple", "number"}));
    ts.Add(MakeProductType("toplevel-variable-binding",
                           {{"variable-name", "string"}, {"bound-expression", "expression"}}));
    ts.Add(MakeVectorType("vector-module-statement", "module-statement"));
    ts.Add(MakeProductType("module", {{"module-statements", "vector-module-statement"}}));
    ts.Add(MakeUnionType("module-statement", {"toplevel-variable-binding"}));
    ts.Add(MakeUnionType("toplevel-node", {"module"}));
}

namespace {

optional<Value> MakeUnionBstNode(TypeStore& ts,
                                 const string& node_type,
                                 const TextAst::Node& n,
                                 const TextAst& textAst)
{
    auto& type = ts.types.at(node_type);
    printf("");
    /*
        if constexpr (VariantAlternativeIdx < std::variant_size_v<AstNode>) {
            using VariantAlternative = std::variant_alternative_t<VariantAlternativeIdx, AstNode>;
            using Spec = AstNodeSpec<VariantAlternative>;
            const auto& rn = textAst.ResolveNode(n);
            if (TestActualCtorForSpec(rn.ctor, Spec::ctor)) {
                if (optional<VariantAlternative> astNode =
                        MakeAstNode<VariantAlternative>(rn, textAst)) {
                    return move(*astNode);
                }
                return nullopt;
            }
            return MakeVariantAstNode<AstNode, VariantAlternativeIdx + 1>(ast_node_name, n,
       textAst); } else { const auto& rn = textAst.ResolveNode(n); if (&rn == &n) {
                fmt::print("Can't construct ast node `{}` with ctor `{}`\n", ast_node_name, n.ctor);
            } else {
                fmt::print("Can't construct ast node `{}` with ctor `{}` (resolved from `{}`)\n",
                           ast_node_name, rn.ctor, n.ctor);
            }
            return nullopt;
        }
        */
    return nullopt;
}

optional<Value> MakeBstNode(TypeStore& ts,
                            const string& node_type,
                            const TextAst::Node& n,
                            const TextAst& textAst);

optional<Value> MakeBstNode(TypeStore& ts, const TextAst::Node& n, const TextAst& textAst)
{
    const auto& rn = textAst.ResolveNode(n);
    if (auto s = TryGetStringLiteral(rn.ctor)) {
        // Implicit string type
        if (!rn.ctor_args.empty()) {
            fmt::print("Implicit string ctor `{}` have non-empty arg list.", rn.ctor);
            return nullopt;
        }
        auto it = ts.types.find("string");
        if (it == ts.types.end()) {
            fmt::print("Unknown bst node type: `string`.\n");
            return nullopt;
        }
        Type* type = &it->second;
        const string& ss = *s;
        return Value::MakeBuiltIn(type, ss);
    }
    auto it = ts.types.find(rn.ctor);
    if (it == ts.types.end()) {
        fmt::print("Unknown bst node type: `{}`\n", rn.ctor);
        return nullopt;
    }
    auto& type = it->second;
    return switch_variant(
        type.desc,
        [&type, &rn](const BuiltInType& t) -> optional<Value> {
            if (type.name == "unit") {
                return Value::MakeBuiltIn(&type, monostate{});
            }
            if (type.name == "number") {
                if (rn.ctor_args.size() != 1) {
                    fmt::print("Initalizing a number without ctor_args.\n");
                    return nullopt;
                }
                auto& first_arg = rn.ctor_args.front();
                if (first_arg.first != "<single-field>") {
                    fmt::print("Initalizing a number with field name `{}`.\n", first_arg.first);
                    return nullopt;
                }
                auto s = TryGetStringLiteral(first_arg.second.ctor);
                if (!s) {
                    fmt::print("Initalizing a number with field ctor `{}`.\n",
                               first_arg.second.ctor);
                    return nullopt;
                }
                return Value::MakeBuiltIn(&type, *s);
            }
            fmt::print("Unknown builtin type: `{}`.\n", type.name);
            return nullopt;
        },
        [&rn](const UnionType& t) -> optional<Value> {
            fmt::print("Can't construct Union type `{}` with a type ctor.\n", rn.ctor);
            return nullopt;
        },
        [&rn, &type, &textAst, &ts](const ProductType& t) -> optional<Value> {
            set<string> field_names_to_initalize;
            for (auto& no : t.named_operands) {
                field_names_to_initalize.insert(no.first);
            }
            map<string, Value> fields;
            struct VectorFieldCollector
            {
                const Type* type = nullptr;
                vector<Value> items;
            };
            map<string, VectorFieldCollector> vector_field_collectors;
            for (auto& arg : rn.ctor_args) {
                const auto* field_name = &arg.first;
                const TextAst::Node& field_initializer_node = arg.second;
                if (arg.first == "<single-field>") {
                    if (t.named_operands.size() == 1) {
                        field_name = &t.named_operands.begin()->first;
                    } else {
                        fmt::print(
                            "Field name is <single-field> but there are {} fields in type `{}`.\n",
                            t.named_operands.size(), type.name);
                        return nullopt;
                    }
                }
                auto it = t.named_operands.find(*field_name);
                if (it == t.named_operands.end()) {
                    fmt::print("Product type `{}` has no field `{}`.\n", rn.ctor, *field_name);
                    return nullopt;
                }

                const string& field_type_name = it->second;
                const auto& field_type = ts.types.at(field_type_name);
                if (holds_alternative<VectorType>(field_type.desc) &&
                    field_initializer_node.ctor != field_type.name) {
                    auto& vector_type = std::get<VectorType>(field_type.desc);
                    auto field_value =
                        MakeBstNode(ts, vector_type.value_type, field_initializer_node, textAst);
                    if (!field_value) {
                        return nullopt;
                    }
                    auto& c = vector_field_collectors[*field_name];
                    if (c.type) {
                        assert(c.type == &field_type);
                    } else {
                        c.type = &field_type;
                    }
                    c.items.emplace_back(move(*field_value));
                } else {
                    auto field_value =
                        MakeBstNode(ts, field_type_name, field_initializer_node, textAst);
                    if (!field_value) {
                        return nullopt;
                    }
                    auto itBool = fields.insert(make_pair(*field_name, move(*field_value)));
                    if (!itBool.second) {
                        fmt::print("Duplicated ctor_arg `{}` for Product type `{}`.\n", *field_name,
                                   type.name);
                        assert(field_names_to_initalize.count(*field_name) == 0);
                        return nullopt;
                    } else {
                        assert(field_names_to_initalize.count(*field_name) > 0);
                        field_names_to_initalize.erase(*field_name);
                    }
                }
            }
            for (auto& c : vector_field_collectors) {
                auto itBool = fields.insert(
                    make_pair(c.first, Value::MakeVector(ts, c.second.type, move(c.second.items))));
                if (!itBool.second) {
                    fmt::print("Duplicated ctor_arg `{}` for Product type `{}`.\n", c.first,
                               type.name);
                    assert(field_names_to_initalize.count(c.first) == 0);
                    return nullopt;
                } else {
                    assert(field_names_to_initalize.count(c.first) > 0);
                    field_names_to_initalize.erase(c.first);
                }
            }
            vector<string> field_names_initialized_now;
            for (auto& field_name : field_names_to_initalize) {
                const string& type_name = t.named_operands.at(field_name);
                const Type& type = ts.types.at(type_name);
                switch_variant(
                    type.desc, [](const BuiltInType&) {},
                    [&field_name, &field_names_initialized_now, &ts, &fields,
                     &type](const UnionType& t) {
                        for (auto& o : t.operands) {
                            if (o == "unit") {
                                field_names_initialized_now.emplace_back(field_name);
                                fields.insert(make_pair(
                                    field_name,
                                    Value::MakeUnion(&type, Value::MakeBuiltIn(&ts.types.at("unit"),
                                                                               monostate{}))));
                                break;
                            }
                        }
                    },
                    [](const ProductType&) {},
                    [&field_name, &field_names_initialized_now, &fields, &type,
                     &ts](const VectorType& t) {
                        field_names_initialized_now.emplace_back(field_name);
                        fields.insert(make_pair(field_name, Value::MakeVector(ts, &type, {})));
                    });
            }
            for (auto& n : field_names_initialized_now) {
                field_names_to_initalize.erase(n);
            }
            if (!field_names_to_initalize.empty()) {
                fmt::print(
                    "Not all fields are initialized for Product type `{}`, unitialized_fields: "
                    "{}.\n",
                    type.name, field_names_to_initalize);
                return nullopt;
            }
            return Value::MakeProduct(&type, move(fields));
        },
        [&rn](const VectorType& t) -> optional<Value> {
            fmt::print("Can't construct Union type `{}` with a type ctor.\n", rn.ctor);
            return nullopt;
        });
}

optional<Value> MakeBstNode(TypeStore& ts,
                            const string& node_type,
                            const TextAst::Node& n,
                            const TextAst& textAst)
{
    const auto& rn = textAst.ResolveNode(n);
    auto sub_value = MakeBstNode(ts, n, textAst);

    if (!sub_value) {
        return nullopt;
    }

    if (node_type == sub_value->type->name) {
        return move(*sub_value);
    }

    auto it = ts.types.find(node_type);
    if (it == ts.types.end()) {
        fmt::print("Unknown bst node type: `{}`\n", node_type);
    }
    auto& type = it->second;

    return switch_variant(
        type.desc,
        [](const BuiltInType& t) -> optional<Value> {
            assert(false);
            return nullopt;
        },
        [&type, &sub_value](const UnionType& t) -> optional<Value> {
            if (t.operands.count(sub_value->type->name) == 0) {
                fmt::print("Union type `{}` has no member `{}`.\n", type.name,
                           sub_value->type->name);
                return nullopt;
            }
            return Value::MakeUnion(&type, move(*sub_value));
        },
        [](const ProductType& t) -> optional<Value> {
            assert(false);
            return nullopt;
        },
        [&type, &sub_value](const VectorType& t) -> optional<Value> {
            if (t.value_type != sub_value->type->name) {
                fmt::print("Vector type `{}` can accept items of type `{}`.\n", type.name,
                           sub_value->type->name);
                return nullopt;
            }
            assert(false);
            return nullopt;
        });
    /*
    switch (type.op) {
        case TypeOp::BuiltIn:
        case TypeOp::Product:
        case TypeOp::Intersection:
            fmt::print("Value of {} type {} an't take a different type ctor {}.\n",
                       to_string(type.op), node_type, rn.ctor);
            return nullopt;
        case TypeOp::Union: {
            auto it = type.operands.find(rn.ctor);
            if (it == type.operands.end()) {
                fmt::print("Union type {} doesn't contain type {}.\n", node_type, rn.ctor);
                return nullopt;
            }

            map<string, Value> operands;
            operands[rn.ctor] = move(*sub_value);
            return Value{&type, move(operands)};
        }
        case TypeOp::Vector:
            assert(false);
            break;
    }
    if (type.op == TypeOp::Union) {
        return MakeUnionBstNode(ts, node_type, n, textAst);
    } else {
        return MakeSimpleBstNode(ts, node_type, n, textAst);
    }*/
}
}  // namespace

struct ComputationPlan {
};

ComputationPlan CallExpression(const Value& v, const vector<Value>& args)
{
    if (v.type->name == "lambda-abstraction") {
        // evaluate arguments
        // create new evcontext with evaluated args
        // create computation plan to evaluate body
    }
    if (v.type->name == "let-expression") {
    }
    if (v.type->name == "function-application") {
    }
    if (v.type->name == "expression-sequence") {
    }
    if (v.type->name == "string") {
    }
    if (v.type->name == "variable") {
    }
    if (v.type->name == "tuple") {
    }
    if (v.type->name == "number") {
    }
    assert(false);
}

optional<Bst> MakeBstFromTextAst(const TextAst& textAst)
{
    Bst bst;
    for (auto& def : textAst.definitions) {
        auto toplevelName = def.first;
        auto x = MakeBstNode(bst.ts, "toplevel-node", def.second, textAst);
        if (x) {
            auto il = x->ToIndentedLines();
            // fmt::print("{}", FormatIndentedLines(il, true));
            do {
                auto module = x->Select("module");
                if (!module) {
                    break;
                }
                auto module_statements = (*module)->Field("module-statements");
                if (!module_statements) {
                    break;
                }
                auto module_statement = (*module_statements)->AtIndex(0);
                if (!module_statement) {
                    break;
                }
                auto tvb = (*module_statement)->Select("toplevel-variable-binding");
                if (!tvb) {
                    break;
                }
                auto vn = (*tvb)->Field("variable-name");
                fmt::print("{}: ",
                           FormatIndentedLines(
                               (*tvb)->Field("variable-name").value()->ToIndentedLines(), true));
                auto be = (*tvb)->Field("bound-expression");
                if (!be) {
                    break;
                }
                CallExpression(
                    **be,
                    vector<Value>({Value::MakeBuiltIn(&bst.ts.types.at("unit"), monostate{})}));
            } while (0);
        }
    }
    return nullopt;
}

}  // namespace snl
