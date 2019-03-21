#pragma once

#include <variant>

namespace forrest {

template <class X, class Y>
using either = std::variant<X, Y>;

template <class X, class Y>
bool is_left(const either<X, Y>& v)
{
    return v.index() == 0;
}

template <class X, class Y>
bool is_right(const either<X, Y>& v)
{
    return v.index() == 1;
}

template <class X, class Y>
const X& left(const either<X, Y>& v)
{
    return std::get<0>(v);
}

template <class X, class Y>
const Y& right(const either<X, Y>& v)
{
    return std::get<1>(v);
}

template <class X, class Y>
X& left(either<X, Y>& v)
{
    return std::get<0>(v);
}

template <class X, class Y>
Y& right(either<X, Y>& v)
{
    return std::get<1>(v);
}
}  // namespace forrest
