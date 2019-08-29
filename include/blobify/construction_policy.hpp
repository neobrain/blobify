#ifndef BLOBIFY_CONSTRUCTION_POLICY_HPP
#define BLOBIFY_CONSTRUCTION_POLICY_HPP

#include "endian.hpp"

#include <type_traits>

namespace blob {

/**
 * Describes how to de-/serialize elementary types from/to a storage_backend.
 *
 * This introduces an intermediary called @a Representative between the
 * storage-provided byte data and the user-requested/provided value.
 * This allows for generic value-based transformations such as byte-swapping
 * that may not be applicable to the final value and that can't easily be
 * performed on raw bytes, such as byte swapping.
 */
struct construction_policy {
    template<typename T, typename Representative, endian SourceEndianness>
    static T decode(Representative source);

    template<typename Representative, typename T, endian TargetEndianness>
    static Representative encode(const T& value);
};

namespace detail {

/**
 * Performs simple source<->host endianness conversion
 *
 * With this default policy, T must be convertible from/to Representative.
 */
struct default_construction_policy : construction_policy {
    template<typename T, typename Representative, endian SourceEndianness>
    static T decode(Representative source) {
        static_assert(SourceEndianness == endian::native,
                      "Endianness conversion not currently supported by DefaultConstructionPolicy");
        return T { source };
    }

    template<typename Representative, typename T, endian TargetEndianness>
    static Representative encode(const T& value) {
        static_assert(TargetEndianness == endian::native,
                      "Endianness conversion not currently supported by DefaultConstructionPolicy");
        if constexpr (std::is_enum_v<T>) {
            // Directly cast enum to integer
            return static_cast<Representative>(value);
        } else {
            // Use brace-initialization to allow for constructors to be called (if any)
            return Representative { value };
        }
    }
};

} // namespace detail

} // namespace blob

#endif // BLOBIFY_CONSTRUCTION_POLICY_HPP
