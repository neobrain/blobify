#ifndef BLOBIFY_ENDIAN_HPP
#define BLOBIFY_ENDIAN_HPP

#if defined(__cpp_lib_endian)
// C++20 provides std::endian
#include <bit>
#elif defined(__APPLE__)
#include <machine/endian.h>
#elif !defined(_WIN32)
#include <endian.h>
#endif

namespace blob {

#if defined(__cpp_lib_endian)
using endian = std::endian;
#else
enum class endian {
#ifdef _WIN32
    little = 0,
    big = 1,
    native = little
#else
    little = __ORDER_LITTLE_ENDIAN__,
    big = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
#endif
};
#endif

} // namespace blob

#endif // BLOBIFY_ENDIAN_HPP
