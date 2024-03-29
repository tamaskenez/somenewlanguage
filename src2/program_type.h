#pragma once

#include "common.h"
#include "type_ptr.h"

namespace snl {

namespace pt {

struct BuiltIn;
struct Union;
struct Intersection;
struct Product;
struct Sum;
struct Function;

struct Unknown
{
    bool operator==(const Unknown& y) const { return true; }
};

struct BuiltIn
{
    enum class Type
    {
        Bottom,
        Unit,
        Top
    };
    Type type;
    bool operator==(const BuiltIn& y) const { return type == y.type; }
};

struct Union
{
    unordered_set<TypePtr> operands;
    bool operator==(const Union& y) const { return operands == y.operands; }
};

struct Intersection
{
    unordered_set<TypePtr> operands;
    bool operator==(const Intersection& y) const { return operands == y.operands; }
};

struct Product
{
    unordered_map<string, TypePtr> operands;
    bool operator==(const Product& y) const { return operands == y.operands; }
};

struct Sum
{
    unordered_map<string, TypePtr> operands;
    bool operator==(const Sum& y) const { return operands == y.operands; }
};

struct Function
{
    TypePtr domain;
    TypePtr codomain;
    bool operator==(const Function& y) const
    {
        return domain == y.domain && codomain == y.codomain;
    }
};

using Type = variant<Unknown, BuiltIn, Union, Intersection, Product, Sum, Function>;

struct TypeWrapper
{
    Type type;
    bool operator==(const TypeWrapper& y) const { return type == y.type; }
};

}  // namespace pt
}  // namespace snl

namespace std {

template <>
struct hash<snl::pt::Unknown>
{
    size_t operator()(snl::pt::Unknown const&) const noexcept { return 0x06ae0; }
};

template <>
struct hash<snl::pt::BuiltIn>
{
    size_t operator()(snl::pt::BuiltIn const&) const noexcept { return 0x0160f; }
};

template <>
struct hash<snl::pt::Union>
{
    size_t operator()(snl::pt::Union const& s) const noexcept
    {
        size_t h(0);
        for (auto& v : s.operands) {
            h ^= hash<snl::pt::TypePtr>{}(v);
        }
        return h;
    }
};

template <>
struct hash<snl::pt::Intersection>
{
    size_t operator()(snl::pt::Intersection const& s) const noexcept
    {
        size_t h(0);
        for (auto& v : s.operands) {
            h ^= hash<snl::pt::TypePtr>{}(v);
        }
        return h;
    }
};

template <>
struct hash<pair<string, snl::pt::TypePtr>>
{
    size_t operator()(pair<string, snl::pt::TypePtr> const& s) const noexcept
    {
        return hash<string>{}(s.first) ^ hash<snl::pt::TypePtr>{}(s.second);
    }
};

template <>
struct hash<snl::pt::Product>
{
    size_t operator()(snl::pt::Product const& s) const noexcept
    {
        size_t h(0);
        for (auto& v : s.operands) {
            h ^= hash<pair<string, snl::pt::TypePtr>>{}(v);
        }
        return h;
    }
};

template <>
struct hash<snl::pt::Sum>
{
    size_t operator()(snl::pt::Sum const& s) const noexcept
    {
        size_t h(0);
        for (auto& v : s.operands) {
            h ^= hash<pair<string, snl::pt::TypePtr>>{}(v);
        }
        return h;
    }
};

template <>
struct hash<snl::pt::Function>
{
    size_t operator()(snl::pt::Function const& s) const noexcept
    {
        return hash<snl::pt::TypePtr>{}(s.domain) ^ hash<snl::pt::TypePtr>{}(s.codomain);
    }
};

template <>
struct hash<snl::pt::TypeWrapper>
{
    size_t operator()(snl::pt::TypeWrapper const& s) const noexcept
    {
        return hash<snl::pt::Type>{}(s.type);
    }
};

}  // namespace std

namespace snl {
namespace pt {
struct Store
{
    Store();
    TypePtr MakeCanonical(BuiltIn&& t);
    TypePtr MakeCanonical(Function&& t);
    bool IsCanonical(TypePtr x) const;

    unordered_set<TypeWrapper> canonical_types;

    TypePtr bottom, top, unit;

private:
    TypePtr MakeCanonicalCommon(Type&& t);
};

}  // namespace pt

}  // namespace snl
