#ifndef BLOBIFY_IS_ARRAY_HPP
#define BLOBIFY_IS_ARRAY_HPP

#include <array>
#include <cstddef>
#include <type_traits>

namespace blobify::detail {

template<typename T>
struct is_std_array : std::false_type {};

template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template<typename T>
inline constexpr auto is_std_array_v = is_std_array<T>::value;

} // namespace blobify::detail

#endif // BLOBIFY_IS_ARRAY_HPP
