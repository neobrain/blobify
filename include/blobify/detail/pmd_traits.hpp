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

} // namespace blobify::detail

#endif // BLOBIFY_PMD_TRAITS_HPP
