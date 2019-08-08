#ifndef BLOBIFY_STORAGE_BACKEND_HPP
#define BLOBIFY_STORAGE_BACKEND_HPP

#include <cstddef>

namespace blob {

/**
 * Defines read/write primitives for interacting with the underlying data source/target
 */
struct storage_backend {
    void skip(std::size_t num_bytes);

    void load(std::byte* target, std::size_t num_bytes);
    void store(std::byte* source, std::size_t num_bytes);

    // TODO: StorageRemaining() for efficient bounds checks?
};

namespace detail {

// Type-erased abstraction for a runtime-provided backend
// TODO: Also buffer on fixed-size array first
struct default_storage_backend : storage_backend {
    // TODO
};

} // namespace detail

} // namespace blob

#endif // BLOBIFY_STORAGE_BACKEND_HPP
