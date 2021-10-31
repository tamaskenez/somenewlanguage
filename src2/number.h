#pragma once

#include <cstdint>

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

private:
    RationalNumber rational;
};

NumberCompareResultOrNaN Compare(const Number& x, const Number& y);

}  // namespace snl
