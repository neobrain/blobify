#ifndef BLOBIFY_LOAD_HPP
#define BLOBIFY_LOAD_HPP

#include "construction_policy.hpp"
#include "properties.hpp"
#include "storage_backend.hpp"

#include <boost/pfr/precise/core.hpp>

#include <cstddef>

namespace blobify {

namespace detail {

// Load a single, plain data type element
template<typename Representative, typename Storage>
inline constexpr Representative load_element_representative(Storage& storage) {
    Representative rep;
    storage.load(reinterpret_cast<char*>(&rep), sizeof(rep));
    return rep;
}

template<auto member_props, typename Member>
inline constexpr Member&& validate_element(Member&& member) {
    if (member_props->expected_value && member != member_props->expected_value) {
        throw std::runtime_error("Input didn't match expected value");
    }

    return std::move(member);
}

// Load a single, plain data type element
template<typename Member, auto member_props, typename Storage, typename ConstructionPolicy>
inline constexpr Member load_element(Storage& storage) {
    using representative_type = typename std::remove_reference_t<decltype(*member_props)>::representative_type;
    auto representative = load_element_representative<representative_type>(storage);
    return validate_element<member_props>(ConstructionPolicy::template decode<Member, representative_type, member_props->endianness>(representative));
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

} // namespace detail

template<typename Data,
         typename Storage = detail::default_storage_backend,
         typename ConstructionPolicy = detail::default_construction_policy>
inline constexpr Data load(Storage&& storage) {
    // NOTE: rvalue reference inputs are forwarded as lvalue references here,
    //       since the Storage will usually carry state that we want to keep
    using members_tuple_t = decltype(boost::pfr::structure_to_tuple(std::declval<Data>()));
    constexpr auto index_sequence = std::make_index_sequence<std::tuple_size_v<members_tuple_t>> { };
    return detail::load_helper_t<Storage, ConstructionPolicy, Data, members_tuple_t>{}(storage, index_sequence);
}

} // namespace blobify

#endif // BLOBIFY_LOAD_HPP
