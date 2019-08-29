#ifndef BLOBIFY_STORE_HPP
#define BLOBIFY_STORE_HPP

#include "construction_policy.hpp"
#include "exceptions.hpp"
#include "properties.hpp"
#include "storage_backend.hpp"

#include "detail/is_array.hpp"

#include <boost/pfr/precise/core.hpp>

#include <cstddef>

namespace blob {

template<typename Storage = detail::default_storage_backend,
         typename ConstructionPolicy = detail::default_construction_policy,
         typename Data>
constexpr void store(Storage&&, const Data& data, tag<ConstructionPolicy> = { });

namespace detail {

// Store a single, plain data type element
template<typename Representative, typename Storage>
constexpr void store_element_representative(Storage& storage, Representative rep) {
    storage.store(reinterpret_cast<std::byte*>(&rep), sizeof(rep));
}

template<auto member_props, typename Storage, typename ConstructionPolicy, typename ArrayType>
constexpr void store_array(Storage&, const ArrayType&);

// Store a single element (possibly aggregate)
template<auto member_props, typename Storage, typename ConstructionPolicy, typename Member>
constexpr void store_element(Storage& storage, const Member& member) {
    if constexpr (detail::is_std_array_v<Member>) {
        // Optimized code path for collections of uniform type
        store_array<member_props, Storage, ConstructionPolicy>(storage, member);
    } else if constexpr (std::is_class_v<Member>) {
        store<Storage&, ConstructionPolicy>(storage, member);
    } else {
        using representative_type = typename std::remove_reference_t<decltype(*member_props)>::representative_type;
        store_element_representative(storage, ConstructionPolicy::template encode<representative_type, Member, member_props->endianness>(member));
    }
}

template<auto member_props, typename Storage, typename ConstructionPolicy, typename ArrayType>
constexpr void store_array(Storage& storage, const ArrayType& array) {
    for (auto& element : array) {
        store_element<member_props, Storage, ConstructionPolicy>(storage, element);
    }
}

template<typename Storage, typename ConstructionPolicy, typename Data, std::size_t... Idxs>
constexpr void store_helper_t(Storage& storage, const Data& data, std::index_sequence<Idxs...>) {
    (store_element<&member_properties_for<Data, Idxs>, Storage, ConstructionPolicy>(storage, boost::pfr::get<Idxs>(data)), ...);
}

/**
 * lens_store but with an explicit base offset parameter
 */
template<typename Value,
         typename ConstructionPolicy,
         auto PointerToMember1,
         auto... PointersToMember,
         typename Storage>
constexpr void lens_store_to_offset(Storage& storage, std::size_t offset, const Value& value) {
    // Skip the up to PointerToMember1, then recurse into the list of variadic parameters
    using Data = typename detail::pmd_traits_t<PointerToMember1>::parent_type;
    constexpr auto index_sequence = std::make_index_sequence<boost::pfr::tuple_size_v<Data>>{};
    constexpr auto member_index = detail::pmd_to_member_index<Data, PointerToMember1>(index_sequence);
    constexpr auto lens_offset = detail::member_offset_for<Data, member_index>();
    offset += lens_offset;

    if constexpr (sizeof...(PointersToMember)) {
        lens_store_to_offset<Value, ConstructionPolicy, PointersToMember...>(storage, offset, value);
    } else {
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
                storage.seek(-static_cast<std::ptrdiff_t>(offset + total_serialized_size<Value>()));
            }
        } seeker(storage, offset);

        // This is the actual object requested by the user, so use the standard load_element code path to fetch it
        constexpr auto& member_properties = detail::member_properties_for<Data, member_index>;
        detail::store_element<&member_properties, Storage, ConstructionPolicy>(storage, value);
    }
}

} // namespace detail

template<typename Storage,
         typename ConstructionPolicy,
         typename Data>
constexpr void store(Storage&& storage, const Data& data, tag<ConstructionPolicy>) {
    detail::generic_validate<Data>();

    // NOTE: rvalue reference Storage inputs are forwarded as lvalue references here,
    //       since the Storage will usually carry state that we want to keep
    constexpr auto index_sequence = std::make_index_sequence<boost::pfr::tuple_size_v<Data>> { };
    detail::store_helper_t<Storage, ConstructionPolicy>(storage, data, index_sequence);
}

template<auto PointerToMember1,
         auto... PointersToMember,
         typename Value,
         typename Storage,
         typename ConstructionPolicy = detail::default_construction_policy>
constexpr void lens_store(Storage&& storage, const Value& value,
                         [[maybe_unused]] tag<ConstructionPolicy> construction_policy_tag = { }) {
    using Data = typename detail::pmd_traits_t<PointerToMember1>::parent_type;
    detail::generic_validate<Data>();
    static_assert(detail::is_valid_pmd_chain_v<Data, decltype(PointerToMember1), decltype(PointersToMember)...>,
                  "Given list of pointers-to-member does not form a valid member lookup chain");

    using SpecificValueType = detail::pointed_member_type<Data, PointerToMember1, PointersToMember...>;

    // Now that we've asserted that the pointer-to-member chain is valid, defer to lens_store_to_offset (which uses a more specific Value parameter type)
    detail::lens_store_to_offset<SpecificValueType, ConstructionPolicy, PointerToMember1, PointersToMember...>(storage, 0, value);
}

} // namespace blob

#endif // BLOBIFY_STORE_HPP
