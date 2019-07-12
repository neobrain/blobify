#ifndef BLOBIFY_PMD_TRAITS_HPP
#define BLOBIFY_PMD_TRAITS_HPP

#include <boost/pfr/precise.hpp>

#include <cstddef>
#include <utility>

#include "../tag.hpp"

namespace blobify::detail {

template<auto PointerToMember>
struct pmd_traits_t;

template<typename T, typename MemberType, MemberType T::*Member>
struct pmd_traits_t<Member> {
    using parent_type = T;
    using member_type = MemberType;
};

template<typename T, auto PointerToMember, std::size_t... Idxs>
constexpr auto pmd_to_member_index(std::index_sequence<Idxs...>) {
    std::size_t index = -1;
    auto t = declval(make_tag<T>);
    // Iterate over all members and check which one matches
    ((static_cast<void*>(&boost::pfr::get<Idxs>(t)) == static_cast<void*>(&(t.*PointerToMember)) && (index = Idxs)), ...);
    return index;
}

template<typename SFINAE, typename Data, typename... PointersToMember>
struct is_valid_pmd_chain_helper : std::false_type {};

template<typename Data, typename... PointersToMember>
struct is_valid_pmd_chain_helper<std::void_t<decltype((std::declval<Data>().* ... .* std::declval<PointersToMember>()))>,
                Data, PointersToMember...>
        : std::true_type {
};

/// Checks if the the given list of pointers-to-member-data yields a well-formed member lookup chain
template<typename Data, typename... PointersToMember>
constexpr bool is_valid_pmd_chain_v = is_valid_pmd_chain_helper<void, Data, PointersToMember...>::value;

} // namespace blobify::detail

#endif // BLOBIFY_PMD_TRAITS_HPP
