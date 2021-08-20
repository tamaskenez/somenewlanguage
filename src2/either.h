#pragma once

#include <variant>

namespace snl {

template <class L, class R>
using either = std::variant<L, R>;

template <class L, class R>
bool is_left(const either<L, R>& v)
{
    return v.index() == 0;
}

template <class L, class R>
bool is_right(const either<L, R>& v)
{
    return v.index() == 1;
}

template <class L, class R>
const L& left(const either<L, R>& v)
{
    return std::get<0>(v);
}

template <class L, class R>
L& left(either<L, R>& v)
{
    return std::get<0>(v);
}

template <class L, class R>
auto get_if_left(either<L, R>* v)
{
    return std::get_if<0>(v);
}

template <class L, class R>
auto get_if_left(const either<L, R>* v)
{
    return std::get_if<0>(v);
}

template <class L, class R>
const R& right(const either<L, R>& v)
{
    return std::get<1>(v);
}

template <class L, class R>
R& right(either<L, R>& v)
{
    return std::get<1>(v);
}

template <class L, class R>
auto get_if_right(either<L, R>* v)
{
    return std::get_if<1>(v);
}

template <class L, class R>
auto get_if_right(const either<L, R>* v)
{
    return std::get_if<1>(v);
}
}  // namespace snl
