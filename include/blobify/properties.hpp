#ifndef BLOBIFY_PROPERTIES_HPP
#define BLOBIFY_PROPERTIES_HPP

#include "tag.hpp"
#include "detail/is_array.hpp"
#include "detail/pmd_traits.hpp"
#include "endian.hpp"

#include <boost/pfr/core.hpp>

#include <cstdint>
#include <cstddef>
#include <optional>

namespace blob {

namespace detail {

/// Helper to map elementary types to the representative type used
template<typename T>
constexpr auto select_representative() {
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


/// Properties of concrete members to load. Parent may be void for standalone data
template<typename T, typename Parent>
struct element_properties_t {
    std::optional<T> expected_value;

    /**
     * Validate enums using a quick check on the enum bounds defined by the smallest and the largest value
     *
     * On validation error, an invalid_enum_value_exception is thrown
     */
    bool validate_enum_bounds = false;

    /**
     * Validate enums by making sure the loaded value is an element of the enum
     *
     * On validation error, an invalid_enum_value_exception is thrown
     */
    bool validate_enum = false;

    struct Dummy {};
    using PointerToSelf = T std::conditional_t<std::is_same_v<Parent, void>, Dummy, Parent>::*;

    // Pointer stored for later reference (e.g. for error reporting)
    PointerToSelf ptr = nullptr;

    /**
     * Endianness in serialized state (endianness for loaded values is configured through @a construction_policy)
     *
     * "native" endianness means the endianness will be copied from the parent here
     */
    endian endianness = endian::native;

    static constexpr bool has_representative_type = (!std::is_class_v<T> && !std::is_union_v<T>) || detail::is_std_array_v<T>;
    /// Type of @a representative passed to construction_policy for decoding/encoding the actual value
    using representative_type = std::conditional_t<has_representative_type,
                                                   // Wrapping MemberType in a conditional_t to prevent select_representative from failing its static_asserts if !has_representative_type
                                                   decltype(detail::select_representative<std::conditional_t<has_representative_type, T, int>>()),
                                                   detail::no_representative_type>;
};

/// Properties of an aggregate
template<typename T>
struct aggregate_properties_t {
    std::size_t expected_size = 0;

    bool expect_tight_packing = false;


    template<std::size_t... Idxs>
    static auto make_members_t(std::index_sequence<Idxs...>) -> std::tuple<element_properties_t<boost::pfr::tuple_element_t<Idxs, T>, T>...>;

    static constexpr std::size_t num_members = boost::pfr::tuple_size_v<T>;

    decltype(make_members_t(std::make_index_sequence<num_members>{})) members;



    template<auto T::*Member>
    constexpr auto& member() {
        constexpr auto lookup_type_t = detail::pmd_to_member_index<T, Member>(std::make_index_sequence<num_members>{});
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

template<typename T>
using properties_t = std::conditional_t<std::is_class_v<T>,
                                        aggregate_properties_t<T>,
                                        element_properties_t<T, void>>;

/**
 * Customization point for specifying properties of user-defined data types
 */
template<typename T>
constexpr auto properties(tag<T>) {
    return properties_t<T> { };
}

namespace detail {

/**
 * Used internally to check that calling properties(make_tag<T>) makes sense
 * to begin with. The user is informed to provide the properties as an explicit
 * parameter if it doesn't.
 */
template<typename T>
constexpr bool has_deducible_properties = std::is_class_v<T>;

// Helper templates to produce pointers that can be passed as template parameters pre-C++20
template<typename Data>
inline constexpr auto properties_for = properties(make_tag<Data>);
template<typename Data, std::size_t Idx>
inline constexpr auto member_properties_for = properties_for<Data>.template member_at<Idx>();

template<typename Data>
constexpr std::size_t total_serialized_size();

template<typename Data, std::size_t Idx>
constexpr std::size_t member_size_for() {
    constexpr auto props = member_properties_for<Data, Idx>;
    if constexpr (props.has_representative_type) {
        constexpr auto representative_size = sizeof(typename decltype(props)::representative_type);

        using member_type = boost::pfr::tuple_element_t<Idx, Data>;
        if constexpr (detail::is_std_array_v<member_type>) {
            return representative_size * std::tuple_size_v<member_type>;
        } else {
            return representative_size;
        }
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
    if constexpr (detail::is_std_array_v<Data>) {
        return std::tuple_size_v<Data> * total_serialized_size<typename Data::value_type>();
    } else if constexpr (std::is_class_v<Data>) {
        constexpr auto size = boost::pfr::tuple_size_v<Data>;
        // Add the size of the last member to its offset
        return member_offset_for<Data, size>();
    } else {
        return sizeof(select_representative<Data>());
    }
}

template<typename Data>
constexpr void generic_validate() {
    constexpr auto props = properties(make_tag<Data>);
    [[maybe_unused]] constexpr auto serialized_size = total_serialized_size<Data>();

    if constexpr (props.expected_size != 0) {
        static_assert(props.expected_size == serialized_size, "Validation failure: Serialized data size does not match the specification");
    }

    if constexpr (props.expect_tight_packing) {
        static_assert (serialized_size == sizeof(Data), "Validation failure: Data type is not tightly packed");
    }
}

} // namespace detail

} // namespace blob

#endif // BLOBIFY_PROPERTIES_HPP
