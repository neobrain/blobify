#ifndef BLOBIFY_PROPERTIES_HPP
#define BLOBIFY_PROPERTIES_HPP

#include "tag.hpp"
#include "detail/pmd_traits.hpp"

#include <optional>

namespace blobify {

template<typename T>
struct properties_t {
    constexpr properties_t() = default;

    template<typename MemberType>
    struct member_property_t {
        std::optional<MemberType> expected_value;

        // Pointer stored for later reference (e.g. for error reporting)
        MemberType T::*ptr = nullptr;
    };

    template<size_t... Idxs>
    static auto make_members_t(std::index_sequence<Idxs...>) -> std::tuple<member_property_t<boost::pfr::tuple_element_t<Idxs, T>>...>;

    decltype(make_members_t(std::make_index_sequence<boost::pfr::tuple_size_v<T>>{})) members;

    // TODO: Provide non-PMD syntax for types that don't specialize blobify::declval
    template<auto T::*Member>
    constexpr auto& member() {
        constexpr auto lookup_type_t = detail::pmd_to_member_index<T, Member>(std::make_index_sequence<boost::pfr::tuple_size_v<T>>{});
        auto& child_property = std::get<lookup_type_t>(members);
        child_property.ptr = Member;
        return child_property;
    }
};

// Default implementation; to be overloaded in user-defined namespaces
template<typename T>
static constexpr auto properties(tag<T>) {
    return properties_t<T> { };
}

} // namespace blobify

#endif // BLOBIFY_PROPERTIES_HPP
