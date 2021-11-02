#pragma once

#include "common.h"

#include <cstdint>
#include <functional>

namespace snl {
enum class NumberCompareResult
{
    Less,
    Equal,
    Greater,
};

enum class NumberCompareResultOrNaN
{
    Less,
    Equal,
    Greater,
    OneOfThemIsNaN,
    BothAreNaNs
};

NumberCompareResultOrNaN ToNumberCompareResultOrNaN(NumberCompareResult x);

// TODO add bigint
struct RationalNumber
{
    const int64_t numerator;
    const int64_t denominator;  // Always positive, nonzero.
    RationalNumber(int64_t numerator, int64_t denominator);
    bool operator==(const RationalNumber& y) const;
};

struct Number
{
    enum class Kind
    {
        Rational,
        NaN
    };
    const Kind kind;

    explicit Number(int64_t i);
    RationalNumber GetRational() const;

    bool operator==(const Number& y) const;

private:
    RationalNumber rational;
};

NumberCompareResultOrNaN Compare(const Number& x, const Number& y);

}  // namespace snl

namespace std {
template <>
struct hash<snl::RationalNumber>
{
    std::size_t operator()(const snl::RationalNumber& x) const noexcept
    {
        auto h = snl::hash_value(x.numerator);
        snl::hash_combine(h, x.denominator);
        return h;
    }
};
template <>
struct hash<snl::Number>
{
    std::size_t operator()(const snl::Number& x) const noexcept
    {
        auto h = snl::hash_value(x.kind);
        switch (x.kind) {
            case snl::Number::Kind::Rational:
                hash_combine(h, x.GetRational());
                break;
            case snl::Number::Kind::NaN:
                break;
        }
        return h;
    }
};

}  // namespace std
