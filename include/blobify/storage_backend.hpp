#ifndef BLOBIFY_STORAGE_BACKEND_HPP
#define BLOBIFY_STORAGE_BACKEND_HPP

#include <cstddef>

namespace blob {

/**
 * Defines read/write primitives for interacting with the underlying data source/target
 */
struct storage_base {
    /**
     * Seeks the stream from the current position
     * @note Implementations that implement both input_storage and output_storage are not required to keep separate read/write cursors
     * @pre num_bytes may not exceed the lower storage bound
     */
    void seek(std::ptrdiff_t num_bytes);
};

struct input_storage : storage_base {

    /**
     * @post The read cursor is advanced by num_bytes elements as if by calling seek(num_bytes)
     * @note Should throw storage_exhausted_exception on error
     */
    void load(std::byte* target, std::size_t num_bytes);
};

struct output_storage : storage_base {
    /**
     * @post The write cursor is advanced by num_bytes elements as if by calling seek(num_bytes)
     * @note Should throw storage_exhausted_exception on error
     */
    void store(std::byte* source, std::size_t num_bytes);

};

namespace detail {

// Type-erased abstraction for a runtime-provided backend
// TODO: Also buffer on fixed-size array first
struct default_storage_backend {
    // TODO
};

} // namespace detail

} // namespace blob

#endif // BLOBIFY_STORAGE_BACKEND_HPP
