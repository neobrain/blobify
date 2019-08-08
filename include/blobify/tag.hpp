#ifndef BLOBIFY_TAG_HPP
#define BLOBIFY_TAG_HPP

#include <type_traits>

namespace blob {

// Tag used to aid overload-resolution
template<typename T>
struct tag { };

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

} // namespace blob

#endif // BLOBIFY_TAG_HPP

