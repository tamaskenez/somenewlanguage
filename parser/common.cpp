#include "common.h"

#include <string>

namespace forrest {
struct Immovable
{
    Immovable(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
    Immovable& operator=(const Immovable&) = delete;
    Immovable& operator=(Immovable&&) = delete;
    Immovable(int a, int b) : a(a), b(b) {}
    int a, b;
};

void test_hash_map_pointer_stability()
{
    // No need to call, if it compiles, OK.
    StableHashMap<std::string, Immovable> hm;
    hm.try_emplace("one", 1, 2);
    hm.rehash(1000);
}
}  // namespace forrest
