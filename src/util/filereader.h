#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <string>

#include "absl/base/optimization.h"
#include "ul/either.h"
#include "ul/inlinevector.h"
#include "ul/maybe.h"

#include "util/constants.h"
#include "util/utf.h"

namespace forrest {

using std::array;
using std::string;
using std::unique_ptr;
using ul::either;
using ul::InlineVector;
using ul::maybe;

class FileReader
{
    static const int FREAD_BUF_SIZE = 32768;
    static const int UTF8_BUF_MIN_SIZE = 32768;
    // If we have <= UTF8_BUF_MIN_SIZE chars in the utf8 buf and
    // we read FREAD_BUF_SIZE then we won't have more than UTF8_BUF_FULL_SIZE
    // utf8 chars there.
    static const int UTF8_BUF_FULL_SIZE = UTF8_BUF_MIN_SIZE + FREAD_BUF_SIZE;
    static const int MAX_UTF_SEQ_SIZE = 4;
    static const int MAX_LEFTOVER_FREAD_BYTES = MAX_UTF_SEQ_SIZE;

    void refill_read_buf();
    bool read_ahead_at_least_unlucky_part(int n);
    void add_utf8(Utf8Char c);
    bool attempt_unlucky(char c);

public:
    static either<string, FileReader> new_(string filename);

    // fow now, only move ctor allowed (add move assignment if needed)
    FileReader(const FileReader&) = delete;

    FileReader(FileReader&& x)
        : f(x.f),
          filename(move(x.filename)),
          utf8_buf(move(x.utf8_buf)),
          next_utf8_to_read(x.next_utf8_to_read),
          utf8_buf_end(x.utf8_buf_end),
          utf8_line(x.utf8_line),
          utf8_col(x.utf8_col),
          in_line(x.in_line),
          in_col(x.in_col),
          last_utf8_added_was_cr(x.last_utf8_added_was_cr),
          no_utf8_chars_added_yet(x.no_utf8_chars_added_yet),
          input_error(move(x.input_error)),
          leftover_fread_bytes(x.leftover_fread_bytes)
    {
        x.f = nullptr;
        x.next_utf8_to_read = nullptr;
        x.utf8_buf_end = nullptr;
    }
    void operator=(const FileReader&) = delete;
    void operator=(FileReader&&) = delete;

    ~FileReader();

    // Buffer may be non-empty when eof.
    bool is_eof() const { return !f; }
    bool is_error() const { return !input_error.empty(); }
    maybe<string> maybe_get_error() const
    {
        if (is_error()) {
            return input_error;
        }
        return {};
    }

    int n_unread_chars() const { return utf8_buf_end - next_utf8_to_read; }

    // Return true if it was possible to ensure n chars.
    bool read_ahead_at_least(int n)
    {
        assert(0 < n && n <= UTF8_BUF_MIN_SIZE);
        if (ABSL_PREDICT_TRUE(n_unread_chars() >= n))
            return true;
        return read_ahead_at_least_unlucky_part(n);
    }

    // Return true if it was possible to ensure n chars.
    bool read_ahead_at_least_1()
    {
        if (ABSL_PREDICT_TRUE(next_utf8_to_read < utf8_buf_end))
            return true;
        return read_ahead_at_least_unlucky_part(1);
    }

    // Refills read_buf if empty then return next char without advancing read
    // position.
    // Return Nothing if eof or error
    maybe<Utf8Char> peek_char()
    {
        if (ABSL_PREDICT_TRUE(read_ahead_at_least_1()))
            return *next_utf8_to_read;
        return {};
    }

    bool attempt(char c)
    {
        if (ABSL_PREDICT_TRUE(next_utf8_to_read < utf8_buf_end)) {
            if (*next_utf8_to_read == c) {
                ++next_utf8_to_read;
                return true;
            } else {
                return false;
            }
        } else {
            return attempt_unlucky(c);
        }
    }

    // Attempt without read-ahead (because already read ahead previously).
    bool attempt_wora(char c)
    {
        assert(next_utf8_to_read < utf8_buf_end);
        if (*next_utf8_to_read == c) {
            ++next_utf8_to_read;
            return true;
        } else {
            return false;
        }
    }

    // Attempt without read-ahead (because already read ahead previously).
    template <class F>
    maybe<Utf8Char> peek_wora(F&& f)
    {
        assert(next_utf8_to_read < utf8_buf_end);
        if (f(*next_utf8_to_read)) {
            return *next_utf8_to_read;
        } else {
            return {};
        }
    }

    // NOTE: Ensure chars in buffer with read_ahead_at_least before calling this!
    Utf8Char peek_char_after_read_ahead(int d) const
    {
        assert(d < n_unread_chars());
        return next_utf8_to_read[d];
    }

    // Refills read_buf if empty then return next char, advances read position.
    // Return nothing if eof or error
    maybe<Utf8Char> next_char()
    {
        if (ABSL_PREDICT_TRUE(read_ahead_at_least_1())) {
            Utf8Char c = *(next_utf8_to_read++);
            if (c == ASCII_CR || c == ASCII_LF) {
                ++utf8_line;
                utf8_col = 1;
            }
            return c;
        }
        return {};
    }

    void skip_whitespace();
    int line() const { return utf8_line; }
    int col() const { return utf8_col; }

private:
    using Utf8Buf = array<Utf8Char, UTF8_BUF_FULL_SIZE>;

    FileReader(FILE* f, string filename);
    void set_input_error(string message);

    FILE* f = nullptr;

public:
    const string filename;

private:
    unique_ptr<Utf8Buf> utf8_buf;
    const Utf8Char* next_utf8_to_read = nullptr;
    Utf8Char* utf8_buf_end = nullptr;

    int utf8_line = 1;
    int utf8_col = 1;

    int in_line = 1;
    int in_col = 1;

    bool last_utf8_added_was_cr = false;
    bool no_utf8_chars_added_yet = true;

    string input_error;

    InlineVector<char, MAX_LEFTOVER_FREAD_BYTES> leftover_fread_bytes;
};  // namespace forrest
}  // namespace forrest
