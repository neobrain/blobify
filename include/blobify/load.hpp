#ifndef BLOBIFY_LOAD_HPP
#define BLOBIFY_LOAD_HPP

#include "construction_policy.hpp"
#include "exceptions.hpp"
#include "properties.hpp"
#include "storage_backend.hpp"

#include "detail/is_array.hpp"

#include <boost/pfr/core.hpp>

#include <magic_enum.hpp>

#include <cstddef>

namespace blob {

namespace detail {

/**
 * Load without error recovery. Contrary to load(), this provides no guarantees
 * about the read/write cursors in error cases.
 */
template<typename Data,
         typename Storage,
         typename ConstructionPolicy>
constexpr Data do_load(Storage& storage, tag<ConstructionPolicy>);

/**
 * Load a single, plain data type element from the current storage offset
 * (or from the given static_offset for random access Storages)
 */
template<typename Representative, typename Storage>
constexpr Representative load_element_representative(Storage& storage) {
    Representative rep;
    storage.load(reinterpret_cast<std::byte*>(&rep), sizeof(rep));
    return rep;
}

// NOTE: Using <algorithm> on std::array in a constexpr context requires instantiation of the array as a global object
template<typename Enum>
inline constexpr auto magic_enum_values_v = magic_enum::enum_values<Enum>();

template<auto member_props, typename Member>
constexpr decltype(auto) validate_element(Member&& member) {
    if constexpr (member_props->expected_value) {
        static_assert(member_props->ptr, "expected_value property is set but the pointer-to-member-data could not be inferred. The pointer must be provided manually in this case.");

        if (member != member_props->expected_value) {
            throw unexpected_value_exception<member_props->ptr>(*member_props->expected_value, member);
        }
    }

    if constexpr (member_props->validate_enum) {
        static_assert (std::is_enum_v<Member>, "validate_enum property is set on a member that is not an enum");

        constexpr auto values = magic_enum::enum_values<Member>();
        if (std::none_of(values.begin(), values.end(), [=](auto val) { return val == member; })) {
            throw invalid_enum_value_exception_for<member_props->ptr>(member);
        }
    }

    if constexpr (member_props->validate_enum_bounds) {
        static_assert (std::is_enum_v<Member>, "validate_enum_bounds property is set on a member that is not an enum");
        static_assert (!member_props->validate_enum, "Setting validate_enum_bounds when validate_enum is already set is redundant");

        auto enum_less = [](auto left, auto right) {
            using type = magic_enum::underlying_type_t<Member>;
            return (static_cast<type>(left) < static_cast<type>(right));
        };
        constexpr auto values_begin = magic_enum_values_v<Member>.begin();
        constexpr auto values_end = magic_enum_values_v<Member>.end();
        constexpr auto minmax_value = std::minmax_element(values_begin, values_end, enum_less);

        // NOTE: The bounds are computed at compile-time
        if (enum_less(member, *minmax_value.first) || enum_less(*minmax_value.second, member)) {
            throw invalid_enum_value_exception_for<member_props->ptr>(member);
        }
    }

    return std::forward<Member>(member);
}

template<typename ElementType, auto member_props, typename Storage, typename ConstructionPolicy, std::size_t NumElements>
constexpr std::array<ElementType, NumElements>
load_array(Storage& storage);

// Load a single element (possibly aggregate)
template<typename Member, auto member_props, typename Storage, typename ConstructionPolicy>
constexpr Member load_element(Storage& storage) {
    if constexpr (detail::is_std_array_v<Member>) {
        // Optimized code path for collections of uniform type
        return load_array<typename Member::value_type, member_props, Storage, ConstructionPolicy, std::tuple_size_v<Member>>(storage);
    } else if constexpr (std::is_class_v<Member>) {
        return do_load<Member, Storage&, ConstructionPolicy>(storage, {});
    } else {
        using representative_type = typename std::remove_reference_t<decltype(*member_props)>::representative_type;
        auto representative = load_element_representative<representative_type>(storage);
        return validate_element<member_props>(ConstructionPolicy::template decode<Member, representative_type, member_props->endianness>(representative));
    }
}

template<typename ElementType, auto member_props, typename Storage, typename ConstructionPolicy, std::size_t... Idxs>
constexpr auto load_array_elementwise(Storage& storage, std::index_sequence<Idxs...>) {
    constexpr auto num_elements = sizeof...(Idxs);
    using ArrayType = std::array<ElementType, num_elements>;
    return ArrayType { { ((void)Idxs, load_element<ElementType, member_props, Storage, ConstructionPolicy>(storage))... } };
}

template<typename ElementType, auto member_props, typename Storage, typename ConstructionPolicy, std::size_t NumElements>
constexpr std::array<ElementType, NumElements>
load_array(Storage& storage) {
    using ArrayType = std::array<ElementType, NumElements>;
    if constexpr (NumElements > 8 && std::is_default_constructible_v<ElementType>) {
        // For a large-ish array, prefer allocating it on stack and
        // initializing it using a loop, since doing so is much easier
        // on the compiler
        ArrayType array { };
        for (auto& element : array) {
            element = load_element<ElementType, member_props, Storage, ConstructionPolicy>(storage);
        }
        return array;
    } else {
        return load_array_elementwise<ElementType, member_props, Storage, ConstructionPolicy>(storage, std::make_index_sequence<NumElements>{});
    }
}

template<typename Storage, typename ConstructionPolicy, typename Data, typename Members>
struct load_helper_t;

template<typename Storage, typename ConstructionPolicy, typename Data, typename... Members>
struct load_helper_t<Storage, ConstructionPolicy, Data, std::tuple<Members...>> {
    template<std::size_t... Idxs>
    Data operator()(Storage& storage, std::index_sequence<Idxs...>) const {
        return Data { load_element<Members, &member_properties_for<Data, Idxs>, Storage, ConstructionPolicy>(storage)... };
    }
};

template<typename Data,
         typename Storage,
         typename ConstructionPolicy>
constexpr Data do_load(Storage& storage, tag<ConstructionPolicy>) {
    detail::generic_validate<Data>();

    using members_tuple_t = decltype(boost::pfr::structure_to_tuple(std::declval<Data>()));
    constexpr auto index_sequence = std::make_index_sequence<std::tuple_size_v<members_tuple_t>> { };
    return detail::load_helper_t<Storage, ConstructionPolicy, Data, members_tuple_t>{}(storage, index_sequence);
}

/**
 * lens_load but with an explicit base offset parameter
 */
template<typename Storage,
         typename ConstructionPolicy,
         auto PointerToMember1,
         auto... PointersToMember
         >
constexpr auto lens_load_from_offset(Storage& storage, std::size_t offset,
                                     tag<ConstructionPolicy> = {}) {
    // Skip the up to PointerToMember1, then recurse into the list of variadic parameters
    using Data = typename detail::pmd_traits_t<PointerToMember1>::parent_type;
    constexpr auto index_sequence = std::make_index_sequence<boost::pfr::tuple_size_v<Data>>{};
    constexpr auto member_index = detail::pmd_to_member_index<Data, PointerToMember1>(index_sequence);
    constexpr auto lens_offset = detail::member_offset_for<Data, member_index>();
    offset += lens_offset;

    if constexpr (sizeof...(PointersToMember)) {
        return lens_load_from_offset<Storage, ConstructionPolicy, PointersToMember...>(storage, offset, {});
    } else {
        // This is the actual object requested by the user, so use the standard load_element code path to fetch it
        using MemberType = std::remove_reference_t<decltype(std::declval<Data>().*PointerToMember1)>;

        // Local helper struct to seek to the member and back upon return.
        struct StorageSeeker {
            Storage& storage;
            std::size_t offset;

            StorageSeeker(Storage& storage, std::size_t offset)
                : storage(storage), offset(offset) {
                storage.seek(offset);
            }

            ~StorageSeeker() {
                // Move back to the beginning of the parent aggregate
                storage.seek(-static_cast<std::ptrdiff_t>(offset + total_serialized_size<MemberType>()));
            }
        } seeker(storage, offset);

        constexpr auto& member_properties = detail::member_properties_for<Data, member_index>;
        return detail::load_element<MemberType, &member_properties, Storage, ConstructionPolicy>(storage);
    }
}

} // namespace detail

/**
 * @post Advances the input stream by the serialized size of Data
 */
template<typename Data,
         typename Storage = detail::default_storage_backend,
         typename ConstructionPolicy = detail::default_construction_policy>
constexpr Data load(Storage&& storage, tag<ConstructionPolicy> = { }) {
    static_assert(detail::has_deducible_properties<Data>, "Data properties are not implicitly deducible");
    if constexpr (detail::has_deducible_properties<Data>) {
        using StorageType = std::remove_reference_t<Storage>;
        return detail::do_load<Data, StorageType, ConstructionPolicy>(storage, {});
    }
}

/**
 * Variant of load_many with explicitly provided properties. Use this for
 * loading collections of elementary types, for which properties() generally
 * is not implemented.
 */
template<typename ContainerData,
         const properties_t<typename ContainerData::value_type>* Properties,
         typename Storage,
         typename ConstructionPolicy = detail::default_construction_policy>
constexpr ContainerData load_many_explicit(Storage&& storage, std::size_t count, tag<ConstructionPolicy> = {}) {
    ContainerData container;
    container.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        container.push_back(detail::load_element<typename ContainerData::value_type, Properties, std::remove_reference_t<Storage>, ConstructionPolicy>(storage));
    }
    return container;
}

template<typename ContainerData,
         typename Storage,
         typename ConstructionPolicy = detail::default_construction_policy>
constexpr ContainerData load_many(Storage&& storage, std::size_t count, tag<ConstructionPolicy> tag = {}) {
    using Data = typename ContainerData::value_type;
    static_assert(detail::has_deducible_properties<Data>, "Data properties are not implicitly deducible. Use load_many_explicit instead");
    if constexpr (detail::has_deducible_properties<Data>) {
        constexpr auto Properties = &detail::properties_for<Data>;
        return load_many_explicit<ContainerData, Properties>(storage, count, tag);
    } else {
        return {};
    }
}

/**
 * Loads a single (possibly deeply nested) struct member from the input storage.
 * The member is assumed to be contained in a serialized blob of the parent of
 * PointerToMember1, so lens_load will seek past any preceding members of that
 * aggregate.
 *
 * To allow further member loads, lens_load seeks back to the beginning of the
 * serialized blob upon completion.
 * TODO: Make this behavior configurable for the last lens_load call in a row. Better yet, allow the caller to pass in a "next offset"
 *
 * The intuition here is that lens_load "zooms" into the serialized data blob
 * and brings the requested member into focus.
 *
 * TODO: In the error case, rewind back to the beginning
 */
template<auto PointerToMember1,
         auto... PointersToMember,
         typename Storage,
         typename ConstructionPolicy = detail::default_construction_policy
         >
constexpr auto lens_load(Storage&& storage,
                         tag<ConstructionPolicy> = { }) {
    using Data = typename detail::pmd_traits_t<PointerToMember1>::parent_type;
    detail::generic_validate<Data>();
    static_assert(detail::is_valid_pmd_chain_v<Data, decltype(PointerToMember1), decltype(PointersToMember)...>,
                  "Given list of pointers-to-member does not form a valid member lookup chain");

    return detail::lens_load_from_offset<std::remove_reference_t<Storage>, ConstructionPolicy, PointerToMember1, PointersToMember...>(storage, 0);
}

} // namespace blob

#endif // BLOBIFY_LOAD_HPP
