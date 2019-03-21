#include "util/filereader.h"

#include "ul/check.h"

#include "util/log.h"

namespace forrest {

using std::system_category;

either<system_error, FileReader> FileReader::new_(string filename)
{
    FILE* f = fopen(filename.c_str(), "rb");
    if (f)
        return FileReader(f, move(filename));
    else
        return system_error(errno, system_category());
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
    : read_buf(new ReadBuf), f(f), filename(move(filename))
{
    p.next_char_to_read = p.read_buf_end = &read_buf->front();
}

bool FileReader::advance_if_prefix(const char* q, int size)
{
    const auto bytes_in_buf = p.read_buf_end - p.next_char_to_read;
    if (bytes_in_buf < size || strncmp(q, p.next_char_to_read, size) != 0)
        return false;
    p.next_char_to_read += size;
    return true;
}

int FileReader::read_ahead_at_least(int n)
{
    CHECK(n <= READ_BUF_SIZE);
    const auto bytes_in_buf = p.read_buf_end - p.next_char_to_read;
    if (bytes_in_buf >= n)
        return bytes_in_buf;
    if (bytes_in_buf > 0 && p.next_char_to_read > &read_buf->front()) {
        // move unread slice of read_buf down to &read_buf->front()
        std::copy(p.next_char_to_read, p.read_buf_end, &read_buf->front());
        p.next_char_to_read -= &read_buf->front();
        p.read_buf_end -= p.next_char_to_read - &read_buf->front();
    }
    auto bytes_read = fread((void*)p.read_buf_end, 1, READ_BUF_SIZE - bytes_in_buf, f);
    p.read_buf_end += bytes_read;
    return p.read_buf_end - p.next_char_to_read;
}

maybe<char> FileReader::peek_char_in_read_buf(int i)
{
    if (p.read_buf_end - p.next_char_to_read >= i)
        return (p.next_char_to_read)[i];
    else
        return {};
}
}  // namespace forrest
