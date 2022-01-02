#pragma once
namespace snl {

struct Term;
using TermPtr = Term const*;

namespace term {
struct Variable;
}  // namespace term

struct Parameter
{
    term::Variable const* variable;
    TermPtr expected_type;
    Parameter(term::Variable const* variable, TermPtr expected_type)
        : variable(variable), expected_type(expected_type)
    {}
    bool operator==(const Parameter& y) const
    {
        return variable == y.variable && expected_type == y.expected_type;
    }
};

}  // namespace snl
