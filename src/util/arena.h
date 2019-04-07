#pragma once

#include <deque>
#include <type_traits>

namespace forrest {
using std::deque;

class Arena
{
    static const size_t PAGE_SIZE = 65536;
    static const size_t MAX_SMALL_BLOCK_SIZE = 8192;
    using Page = std::aligned_storage<PAGE_SIZE, 1024>::type;
    deque<Page> pages;
    void* active_page_first_free_byte = nullptr;
    size_t active_page_bytes_left = 0;

public:
    void* allocate_block(size_t size, size_t alignment)
    {
        if (size <= MAX_SMALL_BLOCK_SIZE) {
            if (!active_page_first_free_byte) {
                // Need a new page.
                pages.emplace_back();
                active_page_first_free_byte = &pages.back();
                active_page_bytes_left = PAGE_SIZE;
            }
            if (std::align(alignment, size, active_page_first_free_byte, active_page_bytes_left)) {
                // Allocate from active page.
                auto result = active_page_first_free_byte;
                active_page_first_free_byte = (char*)active_page_first_free_byte + size;
                active_page_bytes_left -= size;
                return result;
            } else {
                // No room in active page, inactivate and retry.
                active_page_first_free_byte = nullptr;
                active_page_bytes_left = 0;
                return allocate_block(size, alignment);
            }
        }
        fprintf(stderr, "Allocating more then %d is not implemented.\n", (int)MAX_SMALL_BLOCK_SIZE);
        std::terminate();
    }

    // Allocate and placement-new.
    template <class T, class... Args>
    T* new_(Args&&... args)
    {
        void* p = allocate_block(sizeof(T), alignof(T));
        return new (p) T(std::forward<Args>(args)...);
    }
};
}  // namespace forrest
