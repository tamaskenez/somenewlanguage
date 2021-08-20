#include "type.h"

namespace snl {

Type MakeBuiltInType(string name)
{
    return Type{move(name), BuiltInType{}};
}

Type MakeUnionType(string name, set<string> operands)
{
    return Type{move(name), UnionType{move(operands)}};
}

Type MakeProductType(string name, map<string, string> named_operands)
{
    return Type{move(name), ProductType{move(named_operands)}};
}

Type MakeVectorType(string name, string value_type)
{
    return Type{move(name), VectorType{move(value_type)}};
}

bool CanBeAssigned(const Type* assignee, const Type* assigned)
{
    if (assignee == assigned) {
        return true;
    }
    return switch_variant(
        assignee->desc, [](const BuiltInType&) { return false; },
        [assigned](const UnionType& t) { return t.operands.count(assigned->name) > 0; },
        [](const ProductType&) { return false; }, [](const VectorType&) { return false; });
}

TypeStore::TypeStore()
{
    Add(MakeBuiltInType("string"));
    Add(MakeBuiltInType("unit"));
}

void TypeStore::Add(Type type)
{
    string name = type.name;
    assert(!name.empty());
    auto itBool = types.insert(make_pair(move(name), move(type)));
    assert(itBool.second);
}

Value Value::MakeBuiltIn(const Type* type, variant<monostate, string> value)
{
    assert(type);
    assert(holds_alternative<BuiltInType>(type->desc));
    return Value{type, make_unique<ValueContentWrapper>(BuiltInValue{move(value)})};
}

Value Value::MakeUnion(const Type* type, Value value)
{
    assert(type);
    auto& union_type = std::get<UnionType>(type->desc);
    assert(union_type.operands.count(value.type->name) > 0);
    return Value{type, make_unique<ValueContentWrapper>(UnionValue{move(value)})};
}

Value Value::MakeProduct(const Type* type, map<string, Value> fields)
{
    assert(type);
    auto& product_type = std::get<ProductType>(type->desc);
    assert(product_type.named_operands.size() == fields.size());
    for (auto& kv : product_type.named_operands) {
        auto it = fields.find(kv.first);
        assert(it != fields.end());
        assert(it->second.type->name == kv.second);
    }
    return Value{type, make_unique<ValueContentWrapper>(ProductValue{move(fields)})};
}

Value Value::MakeVector(TypeStore& ts, const Type* type, vector<Value> items)
{
    assert(type);
    auto& vector_type = std::get<VectorType>(type->desc);
    if (!items.empty()) {
        auto first_type = items.front().type;
        for (auto& i : items) {
            assert(i.type == first_type);
        }
        auto& value_type = ts.types.at(vector_type.value_type);
        assert(CanBeAssigned(&value_type, first_type));
    }
    return Value{type, make_unique<ValueContentWrapper>(VectorValue{move(items)})};
}

const ValueContent& Content(const Value& value)
{
    assert(value.content);
    return value.content->content;
}
ValueContent& Content(Value& value)
{
    assert(value.content);
    return value.content->content;
}

IndentedLines Value::ToIndentedLines() const
{
    return switch_variant(
        content->content,
        [this](const BuiltInValue& v) {
            auto value = switch_variant(
                v.value, [](monostate) -> string { return string(); },
                [](const string& s) -> string { return QuoteStringForCLiteral(s.c_str()); });
            return IndentedLines({fmt::format("{} :: {}", value, type->name)});
        },
        [](const UnionValue& v) { return v.value.ToIndentedLines(); },
        [this](const ProductValue& v) {
            if (v.fields.empty()) {
                return IndentedLines({fmt::format("{{}}::{}", type->name)});
            } else {
                IndentedLines lines = {"{", ChangeIndentation::Indent};
                bool first = true;
                for (const auto& [field_name, field_value] : v.fields) {
                    auto lines_of_value = field_value.ToIndentedLines();
                    assert(!lines_of_value.empty());
                    if (first) {
                        first = false;
                        if (v.fields.size() == 1 && lines_of_value.size() == 1 &&
                            holds_alternative<string>(lines_of_value.front())) {
                            return IndentedLines({fmt::format(
                                "{{ {}: {} }}::{}", field_name,
                                std::get<string>(lines_of_value.front()), type->name)});
                        }
                    }
                    auto b = lines_of_value.begin();
                    auto e = lines_of_value.end();
                    if (b != e && holds_alternative<string>(*b)) {
                        lines.emplace_back(fmt::format("{}: {}", field_name, std::get<string>(*b)));
                        ++b;
                    } else {
                        lines.emplace_back(fmt::format("{}:", field_name));
                    }
                    lines.insert(lines.end(), make_move_iterator(b), make_move_iterator(e));
                }
                lines.emplace_back(ChangeIndentation::Dedent);
                lines.emplace_back(fmt::format("}}::{}", type->name));
                return lines;
            }
        },
        [this](const VectorValue& v) {
            if (v.values.empty()) {
                return IndentedLines({fmt::format("{{}}::{}", type->name)});
            } else {
                IndentedLines lines = {"{", ChangeIndentation::Indent};
                bool first = true;
                for (size_t index = 0; index < v.values.size(); ++index) {
                    auto lines_of_value = v.values[index].ToIndentedLines();
                    assert(!lines_of_value.empty());
                    if (first) {
                        first = false;
                        if (v.values.size() == 1 && lines_of_value.size() == 1 &&
                            holds_alternative<string>(lines_of_value.front())) {
                            return IndentedLines({fmt::format(
                                "{{ [0]: {} }}::{}", std::get<string>(lines_of_value.front()),
                                type->name)});
                        }
                    }
                    auto b = lines_of_value.begin();
                    auto e = lines_of_value.end();
                    if (b != e && holds_alternative<string>(*b)) {
                        lines.emplace_back(fmt::format("[{}]: {}", index, std::get<string>(*b)));
                        ++b;
                    } else {
                        lines.emplace_back(fmt::format("[{}]:", index));
                    }
                    lines.insert(lines.end(), make_move_iterator(b), make_move_iterator(e));
                }
                lines.emplace_back(ChangeIndentation::Dedent);
                lines.emplace_back(fmt::format("}}::{}", type->name));
                return lines;
            }
        });
}

}  // namespace snl
