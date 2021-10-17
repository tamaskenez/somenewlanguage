#pragma once

#include "either.h"

#include "fmt/core.h"
#include "fmt/ranges.h"

#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#define BE(X) std::begin(X), std::end(X)

#define ASSERT_ELSE(C, E) \
    if (!(C)) {           \
        assert(false);    \
        E                 \
    }

#define REF_FROM_OPT_ELSE_RETURN(VAR_NAME, INITIALIZER, RETURN_VALUE) \
    auto _maybe_##VAR_NAME = INITIALIZER;                             \
    if (!_maybe_##VAR_NAME.has_value()) {                             \
        return RETURN_VALUE;                                          \
    }                                                                 \
    auto& VAR_NAME = *_maybe_##VAR_NAME

#define VAL_FROM_OPT_ELSE_RETURN(VAR_NAME, INITIALIZER, RETURN_VALUE) \
    auto _maybe_##VAR_NAME = INITIALIZER;                             \
    if (!_maybe_##VAR_NAME.has_value()) {                             \
        return RETURN_VALUE;                                          \
    }                                                                 \
    auto VAR_NAME = *_maybe_##VAR_NAME

namespace snl {

using std::holds_alternative;
using std::make_move_iterator;
using std::make_optional;
using std::make_pair;
using std::make_tuple;
using std::make_unique;
using std::map;
using std::monostate;
using std::move;
using std::nullopt;
using std::optional;
using std::pair;
using std::set;
using std::string;
using std::string_view;
using std::to_string;
using std::tuple;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::variant;
using std::vector;

// is_variant
template <typename T>
struct is_variant : std::false_type
{};
template <typename... Args>
struct is_variant<variant<Args...>> : std::true_type
{};
template <typename T>
inline constexpr bool is_variant_v = is_variant<T>::value;

// switch_variant

template <typename... Ts>
struct overloaded;

template <class T, class... Ts>
struct overloaded<T, Ts...> : T, overloaded<Ts...>
{
    template <typename U, typename... Us>
    overloaded(U&& u, Us&&... us)
        : T{std::forward<U>(u)}, overloaded<Ts...>(std::forward<Us>(us)...){};

    using T::operator();
    using overloaded<Ts...>::operator();
};

template <typename T>
struct overloaded<T> : T
{
    template <typename U>
    overloaded(U&& u) : T{std::forward<U>(u)}
    {}

    using T::operator();
};

template <typename... Ts>
overloaded<Ts...> make_overloaded(Ts&&... ts)
{
    return overloaded<Ts...>(std::forward<Ts>(ts)...);
}

template <typename... Ts, typename Variant>
auto switch_variant(Variant&& variant, Ts&&... ts)
{
    return std::visit(make_overloaded(std::forward<Ts>(ts)...), std::forward<Variant>(variant));
}

//

bool starts_with(string_view s, string_view prefix);
string to_string(string_view s);

optional<vector<string>> ReadTextFileToLines(string path);
string QuoteStringForCLiteral(const char* s);

// If ctor is a double-quote string with c-like espaced content, get unescaped string without
// enclosing double-quotes.
optional<string> TryGetStringLiteral(const string& ctor);

// ----
// HASH
// ----

template <class T>
std::size_t hash_value(const T& v)
{
    return std::hash<std::decay_t<T>>{}(v);
}

template <class T>
void hash_combine(std::size_t& seed, const T& v)
{
    seed ^= hash_value(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <class It>
std::size_t hash_range(It first, It last)
{
    std::size_t seed = 0;
    for (; first != last; ++first) {
        hash_combine(seed, *first);
    }
    return seed;
}

template <class It>
void hash_range(std::size_t& seed, It first, It last)
{
    for (; first != last; ++first) {
        hash_combine(seed, *first);
    }
}

}  // namespace snl
namespace std {
template <class U, class V>
struct hash<std::pair<U, V>>
{
    std::size_t operator()(const std::pair<U, V>& x) const noexcept
    {
        auto h = snl::hash_value(x.first);
        snl::hash_combine(h, x.second);
        return h;
    }
};
}  // namespace std

// ----

namespace snl {
template <class T>
T make_copy(const T& x)
{
    return x;
}

template <class T, class C>
void insert_into(unordered_set<T>& x, const C& c)
{
    x.insert(std::begin(c), std::end(c));
}

template <class T, class C>
void insert_into(unordered_set<T>& x, C&& c)
{
    x.insert(std::make_move_iterator(std::begin(c)), std::make_move_iterator(std::end(c)));
}

template <class K, class V>
vector<K> keys_as_vector(const unordered_map<K, V>& m)
{
    vector<K> result;
    result.reserve(m.size());
    for (auto it = m.begin(); it != m.end(); ++it) {
        result.push_back(it->first);
    }
    return result;
}

}  // namespace snl
