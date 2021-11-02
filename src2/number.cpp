#include "number.h"

#include <cassert>
#include <cstdlib>
#include <numeric>

namespace snl {

NumberCompareResultOrNaN ToNumberCompareResultOrNaN(NumberCompareResult x)
{
    switch (x) {
        case NumberCompareResult::Less:
            return NumberCompareResultOrNaN::Less;
        case NumberCompareResult::Equal:
            return NumberCompareResultOrNaN::Equal;
        case NumberCompareResult::Greater:
            return NumberCompareResultOrNaN::Greater;
    }
}

RationalNumber::RationalNumber(int64_t numerator, int64_t denominator)
    : numerator(denominator < 0 ? -numerator : numerator), denominator(std::abs(denominator))
{
    assert(denominator != 0);
}

Number::Number(int64_t i) : kind(Kind::Rational), rational(i, 1) {}
RationalNumber Number::GetRational() const
{
    assert(kind == Kind::Rational);
    return rational;
}

NumberCompareResult ComparePositiveRationals(const RationalNumber& x, const RationalNumber& y)
{
    assert(x.numerator > 0 && y.numerator > 0);
    auto lcm = std::lcm(x.denominator, y.denominator);
    auto nx = x.numerator * (lcm / x.denominator);
    auto ny = y.numerator * (lcm / y.denominator);
    if (nx < ny) {
        return NumberCompareResult::Less;
    } else if (nx == ny) {
        return NumberCompareResult::Equal;
    } else {
        return NumberCompareResult::Greater;
    }
}

NumberCompareResult Compare(const RationalNumber& x, const RationalNumber& y)
{
    if (x.numerator < 0) {
        if (y.numerator < 0) {
            return ComparePositiveRationals(RationalNumber(-y.numerator, y.denominator),
                                            RationalNumber(-x.numerator, x.denominator));
        } else {
            return NumberCompareResult::Less;
        }
    } else if (x.numerator == 0) {
        if (y.numerator < 0) {
            return NumberCompareResult::Greater;
        } else if (y.numerator == 0) {
            return NumberCompareResult::Equal;
        } else {
            return NumberCompareResult::Less;
        }
    } else {
        if (y.numerator <= 0) {
            return NumberCompareResult::Greater;
        } else {
            return ComparePositiveRationals(RationalNumber(x.numerator, x.denominator),
                                            RationalNumber(y.numerator, y.denominator));
        }
    }
}

NumberCompareResultOrNaN Compare(const Number& x, const Number& y)
{
    switch (x.kind) {
        case Number::Kind::Rational:
            switch (y.kind) {
                case Number::Kind::Rational:
                    return ToNumberCompareResultOrNaN(Compare(x.GetRational(), y.GetRational()));
                case Number::Kind::NaN:
                    return NumberCompareResultOrNaN::OneOfThemIsNaN;
            }
        case Number::Kind::NaN:
            switch (y.kind) {
                case Number::Kind::Rational:
                    return NumberCompareResultOrNaN::OneOfThemIsNaN;
                case Number::Kind::NaN:
                    return NumberCompareResultOrNaN::BothAreNaNs;
            }
    }
}

bool Number::operator==(const Number& y) const
{
    if (kind != y.kind) {
        return false;
    }
    switch (kind) {
        case Kind::Rational:
            return rational == y.rational;
        case Kind::NaN:
            return true;
    }
}

bool RationalNumber::operator==(const RationalNumber& y) const
{
    return numerator == y.numerator && denominator == y.denominator;
}

}  // namespace snl
