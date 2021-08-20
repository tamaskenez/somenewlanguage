#include "common.h"

#include <fstream>

namespace snl {

bool starts_with(string_view s, string_view prefix)
{
    return s.substr(0, prefix.size()) == prefix;
}

string to_string(string_view s)
{
    return string(s.data(), s.size());
}

optional<vector<string>> ReadTextFileToLines(string path)
{
    std::ifstream input(path);
    if (!input) {
        return nullopt;
    }
    vector<string> result;
    for (string line; std::getline(input, line);) {
        result.emplace_back(move(line));
    }
    return result;
}

string QuoteStringForCLiteral(const char* s)
{
    string result = "\"";
    bool avoid_hexdigit = false;
    for (; *s; ++s) {
        unsigned char c = *s;
        if (isprint((unsigned char)c)) {
            if (avoid_hexdigit && isxdigit(c)) {
                result += "\"\"";
            }
            avoid_hexdigit = false;
            switch (c) {
                case '\\':
                    result += "\\\\";
                    break;
                case '"':
                    result += "\\\"";
                    break;
                default:
                    result += c;
                    break;
            }
        } else {
            avoid_hexdigit = false;
            switch (c) {
                case '\a':
                    result += "\\a";
                    break;
                case '\b':
                    result += "\\b";
                    break;
                case '\f':
                    result += "\\f";
                    break;
                case '\n':
                    result += "\\n";
                    break;
                case '\r':
                    result += "\\r";
                    break;
                case '\t':
                    result += "\\t";
                    break;
                case '\v':
                    result += "\\v";
                    break;
                default:
                    result += fmt::format("{:#x}", c);
                    avoid_hexdigit = true;
                    break;
            }
        }
    }
    result += '"';
    return result;
}

optional<string> Unescape(string_view s)
{
    string result;
    while (!s.empty()) {
        char c = s[0];
        s.remove_prefix(1);
        if (c == '\\') {
            if (s.empty()) {
                return nullopt;
            }
            c = s[0];
            s.remove_prefix(1);
            switch (c) {
                case '\\':
                    result += '\\';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case 't':
                    result += '\t';
                    break;
                case '"':
                    result += '"';
                    break;
                default:
                    fmt::print("Invalid escape sequence, backslash + char {:#x}\n",
                               (int)(unsigned char)c);
                    return nullopt;
            }
        } else {
            result += c;
        }
    }
    return result;
}

optional<string> TryGetStringLiteral(const string& ctor)
{
    assert(!ctor.empty());
    if (ctor[0] != '"') {
        return nullopt;
    }
    assert(ctor.size() >= 2 && ctor.back() == '"');
    return Unescape(string_view(ctor.data() + 1, ctor.size() - 2));
}

}  // namespace snl
