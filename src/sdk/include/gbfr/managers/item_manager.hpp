// SPDX-License-Identifier: MIT
// gbfr/managers/item_manager.hpp
//
// `ItemManager` has no vftable. Name string at `s_ItemManager`
// (RVA 0x1451b1c29). Public API surface visible from lambda RTTI:
//   ItemManager::createRewardBySet(
//       Reward const&, Behavior*, EntityHandle, int,
//       Hw::cVec4 const*, Hw::cVec4 const*, float,
//       bool, ItemManager::AppearFrom, bool, bool,
//       cycle::reward::RewardType);
#pragma once

#include "../object.hpp"
#include "../signatures.hpp"

namespace gbfr::managers {

class ItemManager : public GameObject {
public:
    using GameObject::GameObject;

    static constexpr std::string_view kSingletonName = "ItemManager";
    static constexpr Address kNameStringRva = signatures::name_string::kItemManager;

    enum class AppearFrom : std::uint32_t {
        // Exact enumerators not yet recovered.
    };
};

} // namespace gbfr::managers
