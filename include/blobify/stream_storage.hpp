#ifndef BLOBIFY_STREAM_STORAGE_HPP
#define BLOBIFY_STREAM_STORAGE_HPP

#include "exceptions.hpp"
#include "storage_backend.hpp"

namespace blob {

namespace detail {

struct istream_storage {
    std::istream& stream;

    void seek(std::ptrdiff_t num_bytes) {
        stream.ignore(num_bytes);
    }

    void load(std::byte* target, std::size_t num_bytes) {
        stream.read(reinterpret_cast<char*>(target), num_bytes);
        if (!stream) {
            throw storage_exhausted_exception { };
        }
    }
};

struct ostream_storage {
    std::ostream& stream;

    void seek(std::ptrdiff_t num_bytes) {
        stream.seekp(num_bytes, std::ios::cur);
    }

    void store(std::byte* source, std::size_t num_bytes) {
        stream.write(reinterpret_cast<char*>(source), num_bytes);
        if (!stream) {
            throw storage_exhausted_exception { };
        }
    }
};

} // namespace detail

/**
 * Storage backend for std::istream.
 *
 * Note this translates each member-load to an fstream read, so for
 * optimal performance you should proxy this with a memory_storage.
 */
struct istream_storage : detail::istream_storage {
    void seek(std::ptrdiff_t num_bytes) {
        detail::istream_storage::seek(num_bytes);
    }
};

/**
 * Storage backend for std::ostream.
 *
 * Note this translates each member-store to an fstream write, so for
 * optimal performance you should proxy this with a memory_storage.
 */
struct ostream_storage : detail::ostream_storage {
    void seek(std::ptrdiff_t num_bytes) {
        detail::ostream_storage::seek(num_bytes);
    }
};

} // namespace blob

#endif // BLOBIFY_STREAM_STORAGE_HPP
