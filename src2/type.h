#pragma once

#include "common.h"

#include "indented_lines.h"

namespace snl {

struct BuiltInType
{};
struct UnionType
{
    set<string> operands;  // Type names of members of the union.
};
struct ProductType
{
    map<string, string> named_operands;  // field name -> type name
};
struct VectorType
{
    string value_type;
};

struct Type
{
    string name;
    variant<BuiltInType, UnionType, ProductType, VectorType> desc;
};

Type MakeBuiltInType(string name);
Type MakeUnionType(string name, set<string> operands);
Type MakeProductType(string name, map<string, string> named_operands);
Type MakeVectorType(string name, string value_type);

bool CanBeAssigned(const Type* assignee, const Type* assigned);

struct TypeStore
{
    unordered_map<string, Type> types;

    TypeStore();
    void Add(Type type);
};

struct ValueContentWrapper;

struct Value
{
    const Type* type = nullptr;
    unique_ptr<ValueContentWrapper> content;

    static Value MakeBuiltIn(const Type* type, variant<monostate, string> value);
    static Value MakeUnion(const Type* type, Value value);
    static Value MakeProduct(const Type* type, map<string, Value> fields);
    static Value MakeVector(TypeStore& ts, const Type* type, vector<Value> items);

    Value() = delete;
    Value(Value&&) = default;
    Value(const Value&) = delete;
    Value& operator=(Value&&) = delete;
    Value& operator=(const Value&) = delete;

    const ValueContentWrapper& Content() const;
    IndentedLines ToIndentedLines() const;
};

struct BuiltInValue
{
    variant<monostate, string> value;
};
struct UnionValue
{
    Value value;
};
struct ProductValue
{
    map<string, Value> fields;
};
struct VectorValue
{
    vector<Value> values;
};

using ValueContent = variant<BuiltInValue, UnionValue, ProductValue, VectorValue>;
struct ValueContentWrapper
{
    explicit ValueContentWrapper(ValueContent&& vc) : content(move(vc)) {}
    ValueContent content;
};

const ValueContent& Content(const Value& value);
ValueContent& Content(Value& value);

}  // namespace snl
