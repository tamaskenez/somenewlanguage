#include "ast.h"

#include "text_ast.h"

#include <any>
#include <typeinfo>

namespace snl {

namespace {

/*
optional<ast::Expression> MakeExpression(const TextAst::Node& node);

optional<ast::Parameter> MakeParameter(const TextAst::Node& node)
{
    ast::Parameter result;
    if (node.ctor != "parameter") {
        fmt::print("Invalid type for Parameter: `{}`\n", node.ctor);
        return nullopt;
    }
    for (auto& arg : node.ctor_args) {
        if (arg.first == "name") {
            if (auto s = GetStringLiteral(arg.second.ctor)) {
                result.name = move(*s);
            } else {
            }
        } else if (arg.first == "type") {
        } else {
            fmt::print("Invalid field for Parameter: `{}`\n", arg.first);
            return nullopt;
        }
    }
}

optional<ast::LambdaAbstraction> MakeLambdaAbstraction(
    const vector<pair<string, TextAst::Node>>& args)
{
    ast::LambdaAbstraction result;
    for (auto& arg : args) {
        if (arg.first == "parameter") {
            if (auto parameter = MakeParameter(arg.second)) {
                result.parameters.emplace_back(move(*parameter));
            } else {
                return nullopt;
            }
        } else if (arg.first == "body") {
            if (auto body = MakeExpression(arg.second)) {
                result.body = move(*body);
            } else {
                return nullopt;
            }
        } else {
            fmt::print("Invalid field for LambdaAbstraction: `{}`\n", arg.first);
            return nullopt;
        }
    }
    return result;
}

optional<ast::LetExpression> MakeLetExpression(const vector<pair<string, TextAst::Node>>& args) {}
optional<ast::FunctionApplication> FunctionApplication(
    const vector<pair<string, TextAst::Node>>& args)
{}
optional<ast::ExpressionSequence> ExpressionSequence(
    const vector<pair<string, TextAst::Node>>& args)
{}

optional<ast::Expression> MakeExpression(const TextAst::Node& node)
{
    if (node.ctor == "lambda-abstraction") {
        return MakeLambdaAbstraction(node.ctor_args);
    }
    if (node.ctor == "let-expression") {
        return MakeLetExpression(node.ctor_args);
    }
    if (node.ctor == "function-application") {
        return FunctionApplication(node.ctor_args);
    }
    if (node.ctor == "expression-sequence") {
        return ExpressionSequence(node.ctor_args);
    }
    if (auto s = GetStringLiteral(node.ctor)) {
        return StringLiteral{move(*s)};
    }
    fmt::print("Invalid type for Expression: `{}`\n", node.ctor) {}
}

optional<ast::ToplevelVariableBinding> MakeToplevelVariableBinding(
    const vector<pair<string, TextAst::Node>>& args)
{
    ast::ToplevelVariableBinding toplevel_variable_binding;
    for (auto& arg : args) {
        if (arg.first == "variable-name") {
            if (auto s = GetStringLiteral(arg.second.ctor)) {
                toplevel_variable_binding.variable_name = *s;
            } else {
                fmt::print("Invalid type for ToplevelVariableBinding.variable-name: `{}`\n",
                           arg.second.ctor);
                return nullopt;
            }
        } else if (arg.first == "bound-expression") {
            if (auto e = MakeExpression(arg.second)) {
                toplevel_variable_binding.bound_expression = Wrap(*e);
            } else {
                return nullopt;
            }
        } else {
            fmt::print("Invalid field for module-statement: `{}`\n", arg.first);
            return nullopt;
        }
    }
}

optional<ast::Module> MakeModule(const vector<pair<string, TextAst::Node>>& args)
{
    ast::Module module;
    for (auto& arg : args) {
        if (arg.first == "module-statement") {
            if (arg.second.ctor != "toplevel-variable-binding") {
                fmt::print("Invalid type for Module.module-statement: `{}`\n", arg.second.ctor);
                return nullopt;
            }
            if (auto tvb = MakeToplevelVariableBinding(arg.second.ctor_args)) {
                module.statements.emplace_back(move(*tvb));
            } else {
                return nullopt;
            }
        } else {
            fmt::print("Invalid field for module: `{}`\n", arg.first);
            return nullopt;
        }
    }
    return module;
}

optional<ast::ToplevelNode> MakeToplevelNode(const TextAst::Node& n)
{
    if (n.ctor == "module") {
        return MakeModule(n.ctor_args);
    }
    fmt::print("Invalid type for ToplevelNode: `{}`\n", n.ctor);
    return nullopt;
}
*/

template <class NodeType, class ParameterType>
struct AstNodeCtorParameterSpec
{
    using Type = ParameterType;
    static constexpr bool is_vector = false;
    const char* name;
    ParameterType NodeType::*member_pointer;
};

template <class NodeType, class ParameterType>
struct AstNodeCtorVectorParameterSpec
{
    using Type = ParameterType;
    static constexpr bool is_vector = true;
    const char* name;
    vector<ParameterType> NodeType::*member_pointer;
};

template <class AstNode>
struct AstNodeSpec;

// NODE SPECIFICATIONS

template <>
struct AstNodeSpec<ast::Module>
{
    using Node = ast::Module;
    static constexpr const char* ctor = "module";
    static constexpr auto parameters =
        make_tuple(AstNodeCtorVectorParameterSpec<Node, ast::ModuleStatement>{"module-statements",
                                                                              &Node::statements});
};

template <>
struct AstNodeSpec<ast::ToplevelVariableBinding>
{
    using Node = ast::ToplevelVariableBinding;
    static constexpr const char* ctor = "toplevel-variable-binding";
    static constexpr auto parameters =
        make_tuple(AstNodeCtorParameterSpec<Node, string>{"variable-name", &Node::variable_name},
                   AstNodeCtorParameterSpec<Node, ast::Expression>{"bound-expression",
                                                                   &Node::bound_expression});
};

template <>
struct AstNodeSpec<ast::StringLiteral>
{
    using Node = ast::StringLiteral;
    static constexpr const char* ctor = "string-literal";
    static constexpr auto parameters =
        make_tuple(AstNodeCtorParameterSpec<Node, string>{"value", &Node::value});
};

template <>
struct AstNodeSpec<ast::LambdaAbstraction>
{
    using Node = ast::LambdaAbstraction;
    static constexpr const char* ctor = "lambda-abstraction";
    static constexpr auto parameters = make_tuple(
        AstNodeCtorVectorParameterSpec<Node, ast::Parameter>{"parameters", &Node::parameters},
        AstNodeCtorParameterSpec<Node, ast::ExpressionWrapper*>{"body", &Node::body});
};

template <>
struct AstNodeSpec<ast::LetExpression>
{
    using Node = ast::LetExpression;
    static constexpr const char* ctor = "let-expression";
    static constexpr auto parameters = make_tuple(
        AstNodeCtorParameterSpec<Node, string>{"variable-name", &Node::variable_name},
        AstNodeCtorParameterSpec<Node, ast::ExpressionWrapper*>{"initializer", &Node::initializer},
        AstNodeCtorParameterSpec<Node, ast::ExpressionWrapper*>{"body", &Node::body});
};

template <>
struct AstNodeSpec<ast::FunctionApplication>
{
    using Node = ast::FunctionApplication;
    static constexpr const char* ctor = "function-application";
    static constexpr auto parameters = make_tuple(
        AstNodeCtorParameterSpec<Node, ast::ExpressionWrapper*>{"function-expression",
                                                                &Node::function_expression},
        AstNodeCtorVectorParameterSpec<Node, ast::ExpressionWrapper*>{"argument",
                                                                      &Node::arguments});
};

template <>
struct AstNodeSpec<ast::ExpressionSequence>
{
    using Node = ast::ExpressionSequence;
    static constexpr const char* ctor = "expression-sequence";
    static constexpr auto parameters =
        make_tuple(AstNodeCtorVectorParameterSpec<Node, ast::ExpressionWrapper*>{
            "expression", &Node::expressions});
};

template <>
struct AstNodeSpec<ast::Tuple>
{
    using Node = ast::Tuple;
    static constexpr const char* ctor = "tuple";
    static constexpr auto parameters =
        make_tuple(AstNodeCtorVectorParameterSpec<Node, ast::ExpressionWrapper*>{
            "expression", &Node::expressions});
};

template <>
struct AstNodeSpec<ast::Parameter>
{
    using Node = ast::Parameter;
    static constexpr const char* ctor = "parameter";
    static constexpr auto parameters =
        make_tuple(AstNodeCtorParameterSpec<Node, string>{"name", &Node::name},
                   AstNodeCtorParameterSpec<Node, ast::Type>{"type", &Node::type});
};

template <>
struct AstNodeSpec<ast::Unit>
{
    using Node = ast::Unit;
    static constexpr const char* ctor = "unit";
    static constexpr auto parameters = make_tuple();
};

template <>
struct AstNodeSpec<ast::Variable>
{
    using Node = ast::Variable;
    static constexpr const char* ctor = "variable";
    static constexpr auto parameters =
        make_tuple(AstNodeCtorParameterSpec<Node, string>{"value", &Node::value});
};

template <>
struct AstNodeSpec<ast::Number>
{
    using Node = ast::Number;
    static constexpr const char* ctor = "number";
    static constexpr auto parameters =
        make_tuple(AstNodeCtorParameterSpec<Node, string>{"value", &Node::value});
};
// END NODE SPECIFICATIONS

std::any MakeAstNodeWrappedInAny(const std::type_info& node_type_info,
                                 const TextAst::Node& n,
                                 const TextAst& textAst);

template <class AstNode>
optional<AstNode> MakeAstNode(const TextAst::Node& n, const TextAst& textAst)
{
    auto result = MakeAstNodeWrappedInAny(typeid(AstNode), n, textAst);
    if (result.has_value()) {
        return move(*std::any_cast<AstNode>(&result));
    }
    return nullopt;
}

bool TestActualCtorForSpec(const std::string& actual_ctor, const char* spec_ctor)
{
    if (!actual_ctor.empty() && actual_ctor[0] == '"') {
        return strcmp(spec_ctor, "string-literal") == 0;
    }
    return actual_ctor == spec_ctor;
}

template <class AstNode, std::size_t VariantAlternativeIdx = 0>
optional<AstNode> MakeVariantAstNode(const char* ast_node_name,
                                     const TextAst::Node& n,
                                     const TextAst& textAst)
{
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
        return MakeVariantAstNode<AstNode, VariantAlternativeIdx + 1>(ast_node_name, n, textAst);
    } else {
        const auto& rn = textAst.ResolveNode(n);
        if (&rn == &n) {
            fmt::print("Can't construct ast node `{}` with ctor `{}`\n", ast_node_name, n.ctor);
        } else {
            fmt::print("Can't construct ast node `{}` with ctor `{}` (resolved from `{}`)\n",
                       ast_node_name, rn.ctor, n.ctor);
        }
        return nullopt;
    }
}

template <std::size_t TupleElementIndex, class AstNode>
bool TrySettingField(AstNode& astNode,
                     const pair<string, TextAst::Node>& arg,
                     const TextAst& textAst)
{
    using Spec = AstNodeSpec<AstNode>;
    using Parameters = decltype(Spec::parameters);
    const auto& parameters = Spec::parameters;
    if constexpr (TupleElementIndex < std::tuple_size_v<Parameters>) {
        const auto& parameter = std::get<TupleElementIndex>(parameters);
        using Parameter = std::decay_t<decltype(parameter)>;
        if (arg.first == parameter.name) {
            if constexpr (std::is_same_v<typename Parameter::Type, string>) {
                const auto& rn = textAst.ResolveNode(arg.second);
                if (auto s = TryGetStringLiteral(rn.ctor)) {
                    astNode.*(parameter.member_pointer) = move(*s);
                    return true;
                } else {
                    if (&arg.second == &rn) {
                        fmt::print("Can't get string from ctor: `{}`\n", rn.ctor);
                    } else {
                        fmt::print("Can't get string from ctor: `{}` (resolved from `{}`)\n",
                                   rn.ctor, arg.second.ctor);
                    }
                }
            } else {
                if (auto parameter_value =
                        MakeAstNode<typename Parameter::Type>(arg.second, textAst)) {
                    if constexpr (Parameter::is_vector) {
                        (astNode.*(parameter.member_pointer)).emplace_back(move(*parameter_value));
                    } else {
                        astNode.*(parameter.member_pointer) = move(*parameter_value);
                    }
                    return true;
                }
            }
            return false;
        }
        return TrySettingField<TupleElementIndex + 1>(astNode, arg, textAst);
    } else {
        return false;
    }
}

template <class AstNode>
optional<AstNode> MakeSimpleAstNode(const char* ast_node_name,
                                    const TextAst::Node& n,
                                    const TextAst& textAst)
{
    using Spec = AstNodeSpec<AstNode>;
    const auto& rn = textAst.ResolveNode(n);
    if (TestActualCtorForSpec(rn.ctor, Spec::ctor)) {
        AstNode astNode;
        for (auto& arg : rn.ctor_args) {
            if (!TrySettingField<0>(astNode, arg, textAst)) {
                fmt::print("Invalid parameter `{}/{}` when constructing `{}`\n", rn.ctor, arg.first,
                           ast_node_name);
                return nullopt;
            }
        }
        return astNode;
    }
    if (&rn == &n) {
        fmt::print("Can't construct ast node `{}` with ctor `{}`\n", ast_node_name, n.ctor);
    } else {
        fmt::print("Can't construct ast node `{}` with ctor `{}` (resolved from `{}`)\n",
                   ast_node_name, rn.ctor, n.ctor);
    }
    return nullopt;
}

template <class AstNode>
optional<AstNode> MakeAstNode2(const char* ast_node_name,
                               const TextAst::Node& n,
                               const TextAst& textAst)
{
    if constexpr (is_variant_v<AstNode>) {
        return MakeVariantAstNode<AstNode>(ast_node_name, n, textAst);
    } else {
        return MakeSimpleAstNode<AstNode>(ast_node_name, n, textAst);
    }
}

std::any MakeAstNodeWrappedInAny(const std::type_info& node_type_info,
                                 const TextAst::Node& n,
                                 const TextAst& textAst)
{
#define TRY(AST_NODE_TYPE)                                                                \
    if (node_type_info == typeid(ast::AST_NODE_TYPE)) {                                   \
        if (auto result = MakeAstNode2<ast::AST_NODE_TYPE>(#AST_NODE_TYPE, n, textAst)) { \
            return std::any(move(*result));                                               \
        }                                                                                 \
        return {};                                                                        \
    }

    TRY(ToplevelNode)
    TRY(Module)
    TRY(ModuleStatement)
    TRY(ToplevelVariableBinding)
    TRY(StringLiteral)
    TRY(Expression)
    TRY(LambdaAbstraction)
    TRY(Parameter)
    TRY(Type)
    TRY(Unit)
    TRY(LetExpression)
    TRY(FunctionApplication)
    TRY(Variable)
    TRY(ExpressionSequence)
    TRY(Tuple)
    TRY(Number)

    if (node_type_info == typeid(ast::ExpressionWrapper*)) {
        if (auto result = MakeAstNode2<ast::Expression>("Expression", n, textAst)) {
            auto* wrapper = new ast::ExpressionWrapper{move(*result)};  // For now, leak.
            return std::any(wrapper);
        }
        return {};
    }
#undef TRY

    fmt::print("Don't know how to construct type `{}`\n", node_type_info.name());
    return {};
}
}  // namespace

optional<Ast> MakeAstFromTextAst(const TextAst& textAst)
{
    Ast ast;
    for (auto& def : textAst.definitions) {
        auto toplevelName = def.first;
        auto x = MakeAstNode<ast::ToplevelNode>(def.second, textAst);
    }
    return nullopt;
}
}  // namespace snl
