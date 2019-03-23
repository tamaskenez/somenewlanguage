#pragma once

#include <unordered_map>

namespace forrest {

// The address of the pairs must be stable through rehash.
template <class K, class V>
using StableHashMap = std::unordered_map<K, V>;

}  // namespace forrest
