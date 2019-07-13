#ifndef BLOBIFY_MODIFY_HPP
#define BLOBIFY_MODIFY_HPP

#include "load.hpp"
#include "store.hpp"

namespace blobify {

template<auto PointerToMember1,
         auto... PointersToMember,
         typename F,
         typename SourceStorage,
         typename TargetStorage = SourceStorage,
         typename ConstructionPolicy = detail::default_construction_policy>
constexpr void lens_modify(SourceStorage&& source, TargetStorage&& target, F&& f,
                              [[maybe_unused]] blobify::tag<ConstructionPolicy> construction_policy_tag = { }) {
    auto value = std::forward<F>(f)(lens_load<PointerToMember1, PointersToMember...>(std::forward<SourceStorage>(source), construction_policy_tag));
    lens_store<PointerToMember1, PointersToMember...>(std::forward<TargetStorage>(target), value, construction_policy_tag);
}

template<auto PointerToMember1,
         auto... PointersToMember,
         typename F,
         typename Storage,
         typename ConstructionPolicy = detail::default_construction_policy>
constexpr void lens_modify(Storage&& storage, F&& f,
                         [[maybe_unused]] blobify::tag<ConstructionPolicy> construction_policy_tag = { }) {
    // Create a copy of the input storage to get independent read/write pointers, then defer to the version with separate source and target storages
    Storage target_storage = storage;
    lens_modify<PointerToMember1, PointersToMember...>(std::forward<Storage>(storage), std::move(target_storage), std::forward<F>(f), construction_policy_tag);
}

} // namespace blobify

#endif // BLOBIFY_MODIFY_HPP
