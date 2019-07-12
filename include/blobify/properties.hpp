#ifndef BLOBIFY_PROPERTIES_HPP
#define BLOBIFY_PROPERTIES_HPP

#include "tag.hpp"
#include "detail/is_array.hpp"
#include "endian.hpp"

#include <boost/pfr/precise/core.hpp>

#include <cstddef>
#include <optional>

namespace blobify {

namespace detail {

/// Helper to map elementary types to the representative type used
template<typename T>
auto select_representative() {
    static_assert(!std::is_pointer_v<T>, "select_representative may not be called on pointers");

    // Class types and unoipns are not supported (unless it's an std::array)
    static_assert((!std::is_class_v<T> && !std::is_union_v<T>) || detail::is_std_array_v<T>, "select_representative may not be called on class/union types");

    static_assert(!std::is_floating_point_v<T>, "select_representative is not implemented for floating point types, yet");

    if constexpr (detail::is_std_array_v<T>) {
        return select_representative<typename T::value_type>();
    } else if constexpr (std::is_enum_v<T>) {
        return std::underlying_type_t<T> { };
    } else {
        static_assert(std::is_integral_v<T>, "Unexpected non-integral type. This is a bug.");

        // TODO: detect floating points, too!
        if constexpr (std::is_signed_v<T>) {
            if constexpr (sizeof(T) == 1) {
                return int8_t { };
            } else if constexpr (sizeof(T) == 2) {
                return int16_t { };
            } else if constexpr (sizeof(T) == 4) {
                return int32_t { };
            } else if constexpr (sizeof(T) == 8) {
                return int64_t { };
            } else {
                static_assert (!sizeof(T), "Unexpected size of integral type");
            }
        } else {
            if constexpr (sizeof(T) == 1) {
                return uint8_t { };
            } else if constexpr (sizeof(T) == 2) {
                return uint16_t { };
            } else if constexpr (sizeof(T) == 4) {
                return uint32_t { };
            } else if constexpr (sizeof(T) == 8) {
                return uint64_t { };
            } else {
                static_assert (!sizeof(T), "Unexpected size of integral type");
            }
        }
    }
}

struct no_representative_type {};

} // namespace detail

template<typename T>
struct properties_t {
    template<typename MemberType>
    struct member_property_t {
        std::optional<MemberType> expected_value;

        // Pointer stored for later reference (e.g. for error reporting)
        MemberType T::*ptr = nullptr;

        /// Endianness in serialized state (endianness for loaded values is configured through @a construction_policy)
        endian endianness = endian::native;

        static constexpr bool has_representative_type = (!std::is_class_v<MemberType> && !std::is_union_v<MemberType>) || detail::is_std_array_v<MemberType>;
        /// Type of @a representative passed to construction_policy for decoding/encoding the actual value
        using representative_type = std::conditional_t<has_representative_type,
                                                       // Wrapping MemberType in a conditional_t to prevent select_representative from failing its static_asserts if !has_representative_type
                                                       decltype(detail::select_representative<std::conditional_t<has_representative_type, MemberType, int>>()),
                                                       detail::no_representative_type>;
    };

    template<std::size_t... Idxs>
    static auto make_members_t(std::index_sequence<Idxs...>) -> std::tuple<member_property_t<boost::pfr::tuple_element_t<Idxs, T>>...>;

    decltype(make_members_t(std::make_index_sequence<boost::pfr::tuple_size_v<T>>{})) members;

    template<auto T::*Member>
    constexpr auto& member() {
        constexpr auto lookup_type_t = detail::pmd_to_member_index<T, Member>(std::make_index_sequence<boost::pfr::tuple_size_v<T>>{});
        auto& child_property = std::get<lookup_type_t>(members);
        child_property.ptr = Member;
        return child_property;
    }

    template<std::size_t Index>
    constexpr auto& member_at() {
        return std::get<Index>(members);
    }

    template<std::size_t Index>
    constexpr auto& member_at() const {
        return std::get<Index>(members);
    }
};

/**
 * Customization point for specifying properties of user-defined data types
 */
template<typename T>
constexpr auto properties(tag<T>) {
    return properties_t<T> { };
}

namespace detail {

// Helper template to produce pointers that can be passed as template parameters pre-C++20
template<typename Data, std::size_t Idx>
inline constexpr auto member_properties_for = properties(make_tag<Data>).template member_at<Idx>();

template<typename Data>
constexpr std::size_t total_serialized_size();

template<typename Data, std::size_t Idx>
constexpr std::size_t member_size_for() {
    constexpr auto props = member_properties_for<Data, Idx>;
    if constexpr (props.has_representative_type) {
        return sizeof(typename decltype(props)::representative_type);
    } else {
        return total_serialized_size<boost::pfr::tuple_element_t<Idx, Data>>();
    }
}

template<typename Data, std::size_t Idx>
constexpr std::size_t member_offset_for() {
    if constexpr (Idx == 0) {
        return 0;
    } else {
        // Build up offset via recursion
        return member_size_for<Data, Idx - 1>() + member_offset_for<Data, Idx - 1>();
    }
}

template<typename Data>
constexpr std::size_t total_serialized_size() {
    constexpr auto size = boost::pfr::tuple_size_v<Data>;
    // Add the size of the last member to its offset
    return member_offset_for<Data, size>();
}

}

} // namespace blobify

#endif // BLOBIFY_PROPERTIES_HPP
