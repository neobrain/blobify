#ifndef BLOBIFY_EXCEPTIONS_HPP
#define BLOBIFY_EXCEPTIONS_HPP

#include "detail/pmd_traits.hpp"

#include <cstddef>
#include <type_traits>

namespace blob {

// Base exception
struct exception {};

template<auto PointerToMember>
struct unexpected_value_exception : exception {
    using value_type = typename detail::pmd_traits_t<PointerToMember>::member_type;
    value_type expected_value;
    value_type actual_value;

    unexpected_value_exception(value_type expected, value_type actual)
        : expected_value(expected), actual_value(actual) {
    }
};

template<typename Enum>
struct invalid_enum_value_exception : exception {
    using enum_type = Enum;
    enum_type actual_value;

    static_assert (std::is_enum_v<Enum>, "Expected enum type");

    invalid_enum_value_exception(enum_type actual)
        : actual_value(actual) {
    }
};

template<auto PointerToMember>
struct invalid_enum_value_exception_for
        : invalid_enum_value_exception<typename detail::pmd_traits_t<PointerToMember>::member_type> {
    using generic_exception_type = invalid_enum_value_exception<typename detail::pmd_traits_t<PointerToMember>::member_type>;
    using enum_type = typename generic_exception_type::enum_type;

    invalid_enum_value_exception_for(enum_type actual)
        : generic_exception_type(actual) {
    }
};

/**
 * Thrown when a load/store operation attempted to access data outside the storage bounds.
 * This usually indicates the input object (stream, file, ...) was too small to store the
 * requested object from in the first place.
 */
struct storage_exhausted_exception : exception { };

} // namespace blob

#endif // BLOBIFY_EXCEPTIONS_HPP
