#ifndef BLOBIFY_LOAD_HPP
#define BLOBIFY_LOAD_HPP

#include "construction_policy.hpp"
#include "exceptions.hpp"
#include "properties.hpp"
#include "storage_backend.hpp"

#include "detail/is_array.hpp"

#include <boost/pfr/precise/core.hpp>

#include <cstddef>

namespace blobify {

template<typename Data,
         typename Storage = detail::default_storage_backend,
         typename ConstructionPolicy = detail::default_construction_policy>
inline constexpr Data load(Storage&& storage);

namespace detail {

// Load a single, plain data type element
template<typename Representative, typename Storage>
inline constexpr Representative load_element_representative(Storage& storage) {
    Representative rep;
    storage.load(reinterpret_cast<char*>(&rep), sizeof(rep));
    return rep;
}

template<auto member_props, typename Member>
inline constexpr decltype(auto) validate_element(Member&& member) {
    if constexpr (member_props->expected_value) {
        static_assert(member_props->ptr, "expected_value property is set but the pointer-to-member-data could not be inferred. The pointer must be provided manually in this case.");

        if (member != member_props->expected_value) {
            throw unexpected_value_exception<member_props->ptr>(*member_props->expected_value, member);
        }
    }

    return std::forward<Member>(member);
}

template<typename ElementType, auto member_props, typename Storage, typename ConstructionPolicy, std::size_t NumElements>
constexpr std::array<ElementType, NumElements>
load_array(Storage& storage);

// Load a single, plain data type element
template<typename Member, auto member_props, typename Storage, typename ConstructionPolicy>
inline constexpr Member load_element(Storage& storage) {
    if constexpr (detail::is_std_array_v<Member>) {
        // Optimized code path for collections of uniform type
        return load_array<typename Member::value_type, member_props, Storage, ConstructionPolicy, std::tuple_size_v<Member>>(storage);
    } else if constexpr (std::is_class_v<Member>) {
        return load<Member, Storage&, ConstructionPolicy>(storage);
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
        ArrayType array;
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

} // namespace detail

template<typename Data,
         typename Storage,
         typename ConstructionPolicy>
inline constexpr Data load(Storage&& storage) {
    // NOTE: rvalue reference inputs are forwarded as lvalue references here,
    //       since the Storage will usually carry state that we want to keep
    using members_tuple_t = decltype(boost::pfr::structure_to_tuple(std::declval<Data>()));
    constexpr auto index_sequence = std::make_index_sequence<std::tuple_size_v<members_tuple_t>> { };
    return detail::load_helper_t<Storage, ConstructionPolicy, Data, members_tuple_t>{}(storage, index_sequence);
}

} // namespace blobify

#endif // BLOBIFY_LOAD_HPP
