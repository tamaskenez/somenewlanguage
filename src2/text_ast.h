#pragma once

#include "common.h"

namespace snl {

struct TextAst
{
    struct Node
    {
        string ctor;  // Literal, if starts with a double-quote.
        vector<pair<string, Node>> ctor_args;
    };
    vector<pair<string, Node>> definitions;
    unordered_map<string, Node> variables;

    const Node& ResolveNode(const Node& node) const;
};

optional<TextAst> ParseLinesToTextAst(const vector<string>& lines);

}  // namespace snl
