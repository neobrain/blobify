#ifndef BLOBIFY_EXCEPTIONS_HPP
#define BLOBIFY_EXCEPTIONS_HPP

#include "detail/pmd_traits.hpp"

namespace blobify {

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

} // namespace blobify

#endif // BLOBIFY_EXCEPTIONS_HPP
