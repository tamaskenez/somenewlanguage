#pragma once

#include <memory>
#include <unordered_map>

#include "util/filereader.h"

namespace forrest {

using std::unique_ptr;

// The address of the pairs must be stable through rehash.
template <class K, class V>
using StableHashMap = std::unordered_map<K, V>;

class AstBuilder
{
public:
    static unique_ptr<AstBuilder> new_(FileReader& fr);
    virtual ~AstBuilder() {}

    virtual bool parse() = 0;
};

}  // namespace forrest
