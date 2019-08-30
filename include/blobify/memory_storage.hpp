#ifndef BLOBIFY_MEMORY_STORAGE_HPP
#define BLOBIFY_MEMORY_STORAGE_HPP

#include <cstddef>
#include <cstring>
#include <iterator>

namespace blob {

/**
 * Storage backend operating on a block of user-managed memory
 */
struct memory_storage {
    std::byte* current;
    std::byte* buffer_begin;
    std::byte* buffer_end;

    template<typename T, size_t N>
    static constexpr memory_storage OnArray(T (&array)[N]) {
        return memory_storage { reinterpret_cast<std::byte*>(array),
                                reinterpret_cast<std::byte*>(array),
                                reinterpret_cast<std::byte*>(std::end(array)) };
    }

    void seek(std::ptrdiff_t size) {
        current += size;
    }

    void load(std::byte* buffer, size_t size) {
        std::memcpy(buffer, current, size);
        current += size;
    }

    void store(std::byte* buffer, size_t size) {
        std::memcpy(current, buffer, size);
        current += size;
    }
};

} // namespace blob

#endif // BLOBIFY_MEMORY_STORAGE_HPP
