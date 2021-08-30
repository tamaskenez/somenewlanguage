#pragma once

#include "common.h"

namespace snl {

namespace pt {

struct BuiltIn;
struct Union;
struct Intersection;
struct Product;
struct Sum;
struct Function;

using Type = variant<const BuiltIn*,
                     const Union*,
                     const Intersection*,
                     const Product*,
                     const Sum*,
                     const Function*>;

struct BuiltIn
{
    enum class Type
    {
        Bottom,
        Unit,
        Top,
        Integer,
        Variable
    };
    Type const type;
    int const index = -1;
    bool operator==(const BuiltIn& y) const { return type == y.type && index == y.index; }
};

struct Union
{
    unordered_set<Type> const operands;
    bool operator==(const Union& y) const { return operands == y.operands; }
};

struct Intersection
{
    unordered_set<Type> const operands;
    bool operator==(const Intersection& y) const { return operands == y.operands; }
};

struct Product
{
    unordered_map<string, Type> const operands;
    bool operator==(const Product& y) const { return operands == y.operands; }
};

struct Sum
{
    unordered_map<string, Type> const operands;
    bool operator==(const Sum& y) const { return operands == y.operands; }
};

struct Function
{
    Type const domain;
    Type const codomain;
    bool operator==(const Function& y) const
    {
        return domain == y.domain && codomain == y.codomain;
    }
};

}  // namespace pt
}  // namespace snl

namespace std {

template <>
struct hash<snl::pt::BuiltIn>
{
    size_t operator()(snl::pt::BuiltIn const& s) const noexcept
    {
        return hash<int>{}(int(s.type)) ^ hash<int>{}(s.index);
    }
};

template <>
struct hash<snl::pt::Union>
{
    size_t operator()(snl::pt::Union const& s) const noexcept
    {
        size_t h(0);
        for (auto& v : s.operands) {
            h ^= hash<snl::pt::Type>{}(v);
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
            h ^= hash<snl::pt::Type>{}(v);
        }
        return h;
    }
};

template <>
struct hash<pair<string, snl::pt::Type>>
{
    size_t operator()(pair<string, snl::pt::Type> const& s) const noexcept
    {
        return hash<string>{}(s.first) ^ hash<snl::pt::Type>{}(s.second);
    }
};

template <>
struct hash<snl::pt::Product>
{
    size_t operator()(snl::pt::Product const& s) const noexcept
    {
        size_t h(0);
        for (auto& v : s.operands) {
            h ^= hash<pair<string, snl::pt::Type>>{}(v);
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
            h ^= hash<pair<string, snl::pt::Type>>{}(v);
        }
        return h;
    }
};

template <>
struct hash<snl::pt::Function>
{
    size_t operator()(snl::pt::Function const& s) const noexcept
    {
        return hash<snl::pt::Type>{}(s.domain) ^ hash<snl::pt::Type>{}(s.codomain);
    }
};
}  // namespace std

namespace snl {
namespace pt {
struct Store
{
    Store();
    Type MakeCanonical(const BuiltIn& t);
    Type MakeCanonical(const Function& t);
    bool IsCanonical(Type x) const;

    tuple<unordered_set<BuiltIn>,
          unordered_set<Union>,
          unordered_set<Intersection>,
          unordered_set<Product>,
          unordered_set<Sum>,
          unordered_set<Function>>
        type_maps;

    Type bottom, top, unit;
};

}  // namespace pt

}  // namespace snl
