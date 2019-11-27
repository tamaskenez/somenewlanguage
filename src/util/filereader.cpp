#include "util/filereader.h"

#include "absl/strings/str_format.h"
#include "ul/check.h"

#include "util/log.h"

namespace forrest {

using absl::StrFormat;

either<string, FileReader> FileReader::new_(string filename)
{
    FILE* f = fopen(filename.c_str(), "rb");
    if (f)
        return FileReader(f, move(filename));
    else
        return StrFormat("Can't open %s for reading: %s.", filename.c_str(),
                         errno == 0 ? "Unknown error" : strerror(errno));
}

FileReader::~FileReader()
{
    if (f) {
        int r = fclose(f);
        if (r != 0)
            LOG_DEBUG("fclose(\"{}\") -> {}", filename, r);
    }
}

FileReader::FileReader(FILE* f, string filename)
    : f(f),
      filename(move(filename)),
      utf8_buf(new Utf8Buf),
      next_utf8_to_read(utf8_buf->data()),
      utf8_buf_end(utf8_buf->data())
{
}

void FileReader::add_utf8(Utf8Char c)
{
    if (ABSL_PREDICT_FALSE(c == ASCII_CR)) {
        in_col = 1;
        ++in_line;
        last_utf8_added_was_cr = true;
    } else if (ABSL_PREDICT_FALSE(c == ASCII_LF)) {
        if (last_utf8_added_was_cr) {
            last_utf8_added_was_cr = false;
            return;
        }
        in_col = 1;
        ++in_line;
    } else {
        if (ABSL_PREDICT_TRUE(!no_utf8_chars_added_yet)) {
            ++in_col;
            last_utf8_added_was_cr = false;
        } else {
            no_utf8_chars_added_yet = false;
            if (ABSL_PREDICT_TRUE(!c.is_utf8_bom())) {
                ++in_col;
            } else {
                // Ignore UTF-8 BOM
                return;
            }
        }
    }
    assert(utf8_buf_end < utf8_buf->data() + utf8_buf->size());
    *(utf8_buf_end++) = c;
}

void FileReader::refill_read_buf()
{
    for (;;) {
        const auto nuc = n_unread_chars();
        if (nuc >= UTF8_BUF_MIN_SIZE)
            return;
        // Move unread part back to buffer start.
        if (nuc > 0) {
            const Utf8Char* a = next_utf8_to_read;
            const Utf8Char* b = utf8_buf_end;
            std::copy(a, b, utf8_buf->data());
            next_utf8_to_read = utf8_buf->data();
            utf8_buf_end = utf8_buf->data() + nuc;
        }

        array<char, FREAD_BUF_SIZE> fread_buf;

        auto first_byte_to_read_to = fread_buf.data() + leftover_fread_bytes.size();
        std::copy(leftover_fread_bytes.begin(), leftover_fread_bytes.end(), fread_buf.data());

        const auto n_bytes_to_read = f ? FREAD_BUF_SIZE - leftover_fread_bytes.size() : 0;
        if (n_bytes_to_read + leftover_fread_bytes.size() == 0)
            return;
        leftover_fread_bytes.clear();

        const auto n_bytes_read =
            n_bytes_to_read > 0 ? fread((void*)first_byte_to_read_to, 1, n_bytes_to_read, f) : 0;

        if (n_bytes_read < n_bytes_to_read) {
            if (ferror(f)) {
                set_input_error("Can't read file.");
                return;
            } else if (feof(f)) {
                f = nullptr;
            } else {
                set_input_error("No bytes received while reading file.");
                return;
            }
        }

        const auto bytes_end = first_byte_to_read_to + n_bytes_read;
        int utf8_seq_length;
        for (char* p = fread_buf.data(); p < bytes_end; p += utf8_seq_length) {
            char c0 = *p;
            array<char8_t, MAX_UTF_SEQ_SIZE> uc;
            uc[0] = c0;
            if (is_ascii_utf8_byte(c0)) {
                utf8_seq_length = 1;
            } else if (is_leading_byte_of_two_byte_utf8_seq(c0)) {
                utf8_seq_length = 2;
            } else if (is_leading_byte_of_three_byte_utf8_seq(c0)) {
                utf8_seq_length = 3;
            } else if (is_leading_byte_of_four_byte_utf8_seq(c0)) {
                utf8_seq_length = 4;
            } else {
                set_input_error(StrFormat("Invalid UTF-8 leading byte (%02X)", c0));
                return;
            }
            assert(utf8_seq_length <= MAX_UTF_SEQ_SIZE);
            if (utf8_seq_length != 1) {
                if (p + utf8_seq_length > bytes_end) {
                    leftover_fread_bytes.resize(bytes_end - p);
                    std::copy(p, bytes_end, leftover_fread_bytes.data());
                    refill_read_buf();
                    return;
                }
                for (int i = 1; i < utf8_seq_length; ++i) {
                    auto c = p[i];
                    if (is_utf8_continuation_byte(c)) {
                        uc[i] = c;
                    } else {
                        set_input_error(StrFormat("Invalid UTF-8 continuation byte (%02X)", c));
                        return;
                    }
                }
            }
            add_utf8(Utf8Char{uc});
        }
    }
}

void FileReader::set_input_error(string message)
{
    f = nullptr;
    input_error = StrFormat("%s in file %s:%d:%d.", message, filename, in_line, in_col);
}

void FileReader::skip_whitespace()
{
    for (;;) {
        for (; next_utf8_to_read != utf8_buf_end; ++next_utf8_to_read) {
            if (!isspace(next_utf8_to_read->front())) {
                return;
            }
        }
        if (is_eof())
            return;
        refill_read_buf();
    }
}
bool FileReader::read_ahead_at_least_unlucky_part(int n)
{
    if (!f)
        return false;
    refill_read_buf();
    return n_unread_chars() >= n;
}

bool FileReader::attempt_unlucky(char c)
{
    if (ABSL_PREDICT_TRUE(read_ahead_at_least_1()) && *next_utf8_to_read == c) {
        ++next_utf8_to_read;
        return true;
    } else {
        return false;
    }
}

}  // namespace forrest
