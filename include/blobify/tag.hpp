#ifndef BLOBIFY_TAG_HPP
#define BLOBIFY_TAG_HPP

#include <type_traits>

namespace blobify {

// Tag used to find user-provided, namespaced functions via overload resolution
template<typename T>
using tag = T*;

template<typename T>
constexpr auto make_tag = tag<T> { };

// Constructs a placeholder value of the given type
// Like std::declval, but user-customizeable and constexpr
template<typename T>
constexpr auto declval(tag<T>) {
    // NOTE: Should explicitly check if T is constexpr-constructible
    static_assert(std::is_default_constructible_v<T>, "T must be constexpr default-constructible, or declval must be customized");
    return T { };
}

} // namespace blobify

#endif // BLOBIFY_TAG_HPP

