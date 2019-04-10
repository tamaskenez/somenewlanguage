#include "builtinnames.h"
namespace forrest {

using std::make_unique;

BuiltinNames::BuiltinNames()
{
    symbols[FN] = new SymLeaf("fn");
    symbols[DEF] = new SymLeaf("def");
    FOR (i, 0, < COUNT) {
        cstring_to_names[symbols[i]->name] = (NameId)i;
    }
}
SymLeaf* BuiltinNames::id_to_symleaf(NameId name) const
{
    assert(0 <= name && name < COUNT);
    return symbols[name];
}
maybe<BuiltinNames::NameId> BuiltinNames::string_to_id(const string& s) const
{
    auto it = cstring_to_names.find(s);
    if (it == cstring_to_names.end())
        return {};
    return it->second;
}
void BuiltinNames::init_g()
{
    g.reset(new BuiltinNames);
}

unique_ptr<BuiltinNames> BuiltinNames::g;

}  // namespace forrest
