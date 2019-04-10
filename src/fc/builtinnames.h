#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>

#include "ul/usual.h"
#include "util/maybe.h"

#include "ast.h"

namespace forrest {

using std::array;
using std::string;
using std::unique_ptr;
using std::unordered_map;

class BuiltinNames
{
public:
    enum NameId
    {
        FN,
        DEF,
        COUNT
    };

private:
    array<SymLeaf*, COUNT> symbols;
    unordered_map<string, NameId> cstring_to_names;

    BuiltinNames();

public:
    static void init_g();
    static unique_ptr<BuiltinNames> g;

    maybe<NameId> string_to_id(const string& s) const;
    SymLeaf* id_to_symleaf(NameId name) const;
};

}  // namespace forrest
