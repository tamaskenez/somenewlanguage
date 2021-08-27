#include "text_ast.h"

namespace snl {

const TextAst::Node& TextAst::ResolveNode(const Node& node) const
{
    if (node.ctor.empty() || node.ctor[0] != '$') {
        return node;
    }
    auto it = variables.find(node.ctor.substr(1));
    if (it == variables.end()) {
        return node;
    }
    return it->second;
}

namespace {
struct TokenizedLine
{
    string_view indentation;
    vector<string_view> tokens;
};

string_view AcceptSpaceAndTabs(string_view& s)
{
    auto b = s.begin();
    while (!s.empty() && (s[0] == ' ' || s[0] == '\t')) {
        s.remove_prefix(1);
    }
    return string_view(b, s.begin() - b);
}

either<string, std::optional<string_view>> AcceptQuotedString(string_view& s0)
{
    auto s = s0;
    if (s.empty() || s[0] != '"') {
        return nullopt;
    }
    s.remove_prefix(1);
    for (;;) {
        if (s.empty()) {
            return string("String literal without closing double-quote.");
        }
        char c = s[0];
        s.remove_prefix(1);
        switch (c) {
            case '"': {
                auto result = string_view(s0.begin(), s.begin() - s0.begin());
                s0 = s;
                return result;
            }
            case '\\': {
                if (s.empty()) {
                    return string("String literal ends after backslash.");
                }
                s.remove_prefix(1);
            } break;
            default:
                break;
        }
    }
}

optional<string_view> AcceptToken(string_view& s)
{
    auto b = s.begin();
    while (!s.empty() && (s[0] != ' ' && s[0] != '\t')) {
        s.remove_prefix(1);
    }
    auto result = string_view(b, s.begin() - b);
    if (result.empty()) {
        return nullopt;
    }
    return result;
}

optional<TokenizedLine> Tokenize(string_view s0)
{
    auto s = s0;
    TokenizedLine result;
    result.indentation = AcceptSpaceAndTabs(s);
    for (;;) {
        AcceptSpaceAndTabs(s);
        if (s.empty()) {
            return result;
        }
        auto eitherQuotedString = AcceptQuotedString(s);
        if (auto* l = get_if_left(&eitherQuotedString)) {
            fmt::print("Error: {}, in line:\n`{}`\n", *l, s0);
            return nullopt;
        }
        auto& r = right(eitherQuotedString);
        if (r) {
            result.tokens.emplace_back(*r);
            continue;
        }
        if (auto token = AcceptToken(s)) {
            result.tokens.emplace_back(*token);
        }
    }
    return result;
}
enum class Ordering
{
    Less,
    Equal,
    Greater
};

optional<Ordering> CompareIndentations(string_view i, string_view j)
{
    if (i.size() < j.size()) {
        if (starts_with(j, i)) {
            return Ordering::Less;
        }
    } else if (i.size() == j.size()) {
        if (i == j) {
            return Ordering::Equal;
        }
    } else {
        assert(i.size() > j.size());
        if (starts_with(i, j)) {
            return Ordering::Greater;
        }
    }
    return nullopt;
}

}  // namespace

optional<TextAst> ParseLinesToTextAst(const vector<string>& lines)
{
    TextAst textAst;
    struct Level
    {
        TextAst::Node* node = nullptr;
        optional<string_view> childrenIndentation;
    };
    vector<Level> node_stack;
    for (auto& line : lines) {
        auto tokenizedLine = Tokenize(line);
        if (!tokenizedLine) {
            return nullopt;
        }
        if (tokenizedLine->tokens.empty()) {
            // Empty line, go back to zero indentation.
            node_stack.clear();
            continue;
        }

        if (tokenizedLine->indentation.empty()) {
            node_stack.clear();
        } else {
            // tokenizedLine has some nonzero indentation.
            if (node_stack.empty()) {
                fmt::print("Error: Top-level line can't be indented:\n`{}`\n", line);
                return nullopt;
            }
            bool done = false;
            bool first_iteration = true;
            do {
                if (node_stack.back().childrenIndentation) {
                    // We might get here only by backtracking (popping from node_stack).
                    assert(!first_iteration);
                    auto ordering = CompareIndentations(*node_stack.back().childrenIndentation,
                                                        tokenizedLine->indentation);
                    if (!ordering) {
                        fmt::print("Error: Bad indentation in line:\n`{}`\n", line);
                        return nullopt;
                    }
                    switch (*ordering) {
                        case Ordering::Less:
                            // Invalid state, we shouldn't have popped the node_stack.
                            fmt::print(
                                "Error: Internal, children indentation is less then line's, in "
                                "line:\n`{}`\n",
                                line);
                            return nullopt;
                        case Ordering::Equal:
                            // Add to current parent.
                            done = true;
                            break;
                        case Ordering::Greater:
                            node_stack.pop_back();
                            break;
                    }
                    continue;
                }

                first_iteration = false;

                // Previous line had no children.
                if (node_stack.size() == 1) {
                    // Previous line is top-level, any nonzero indentation makes this line a
                    // child of previous line.
                    node_stack.back().childrenIndentation = tokenizedLine->indentation;
                    // We're done.
                    break;
                }

                assert(node_stack.size() >= 2);
                auto& grandParent = node_stack[node_stack.size() - 2];
                auto parent_identation = grandParent.childrenIndentation.value();
                auto ordering = CompareIndentations(parent_identation, tokenizedLine->indentation);
                if (!ordering) {
                    fmt::print("Error: Bad indentation in line:\n`{}`\n", line);
                    return nullopt;
                }
                switch (*ordering) {
                    case Ordering::Less:
                        // Parent indentation is less, make this line child of parent.
                        done = true;
                        break;
                    case Ordering::Equal:
                        // Same as parent indentation, make this line child of grandparent.
                        node_stack.pop_back();
                        done = true;
                        break;
                    case Ordering::Greater:
                        // Parent indentation is greater, go back one level and retry.
                        node_stack.pop_back();
                        break;
                }
            } while (!done);
        }

        auto first_token = tokenizedLine->tokens[0];
        assert(!first_token.empty());
        if (first_token.back() != ':') {
            fmt::print("Error: First token must end with ':', in line:\n`{}`\n", line);
            return nullopt;
        }
        TextAst::Node new_node;
        if (tokenizedLine->tokens.size() == 1) {
            if (first_token[0] == '$') {
                fmt::print(
                    "Error: Variable definition must contain an explicit constructor, in "
                    "line:\n`{}`\n",
                    line);
                return nullopt;
            }
            new_node.ctor = first_token.substr(0, first_token.size() - 1);
        } else if (tokenizedLine->tokens.size() == 2) {
            auto second_token = tokenizedLine->tokens[1];
            assert(!second_token.empty());
            new_node.ctor = second_token;
        } else if (tokenizedLine->tokens.size() == 3) {
            auto second_token = tokenizedLine->tokens[1];
            auto third_token = tokenizedLine->tokens[2];
            assert(!second_token.empty());
            assert(!third_token.empty());
            new_node.ctor = second_token;
            new_node.ctor_args.emplace_back("<single-field>",
                                            TextAst::Node{to_string(third_token)});
        } else {
            fmt::print("Error: Lines must contain 2 or 3 tokens:\n`{}`\n", line);
            return nullopt;
        }
        if (node_stack.empty()) {
            // Top-level.
            if (first_token[0] == '$') {
                // Variable definition.
                auto itBool = textAst.variables.insert(make_pair(
                    to_string(first_token.substr(1, first_token.size() - 2)), move(new_node)));
                if (!itBool.second) {
                    fmt::print("Error: Redefined variable, in line:\n`{}`\n", line);
                    return nullopt;
                }
                node_stack.emplace_back(Level{&itBool.first->second});
            } else {
                // Top-level node.
                textAst.definitions.emplace_back(first_token.substr(0, first_token.size() - 1),
                                                 move(new_node));
                node_stack.emplace_back(Level{&textAst.definitions.back().second});
            }
        } else {
            if (first_token[0] == '$') {
                fmt::print("Error: Variable declaration must be top-level, in line:\n`{}`\n", line);
                return nullopt;
            }
            if (node_stack.back().childrenIndentation) {
                assert(node_stack.back().childrenIndentation == tokenizedLine->indentation);
            } else {
                node_stack.back().childrenIndentation = tokenizedLine->indentation;
            }
            node_stack.back().node->ctor_args.emplace_back(
                first_token.substr(0, first_token.size() - 1), move(new_node));
            node_stack.emplace_back(Level{&node_stack.back().node->ctor_args.back().second});
        }
    }
    return textAst;
}
}  // namespace snl
