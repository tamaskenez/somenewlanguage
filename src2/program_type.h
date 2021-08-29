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
struct Variable;

using Type = variant<BuiltIn*, Union*, Intersection*, Product*, Sum*, Function*>;

struct BuiltIn
{
    enum Type
    {
        Bottom,
        Unit,
        Top,
        Integer
    };
    Type const type;
    int const index = 0;
};

struct Union
{
    unordered_set<Type> const operands;
};

struct Intersection
{
    unordered_set<Type> const operands;
};

struct Product
{
    unordered_map<string, Type> const operands;
};

struct Sum
{
    unordered_map<string, Type> const operands;
};

struct Function
{
    Type const domain;
    Type const codomain;
};

struct Variable
{
    int const index = -1;
};

}  // namespace pt

}  // namespace snl
