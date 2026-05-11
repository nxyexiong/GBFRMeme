// SPDX-License-Identifier: MIT
// gbfr/managers/npc_appearance_manager.hpp
//
// `cNpcAppearanceManager` — vftable @ 0x1447fe390.
// Owns `cAppearsNpcHandle` / `cAppearanceNpc` runtime objects.
#pragma once

#include "../object.hpp"

namespace gbfr::managers {

class NpcAppearanceManager : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr std::string_view kSingletonName = "cNpcAppearanceManager";
    static constexpr Address kVftableRva = 0x1447fe390ULL;
};

} // namespace gbfr::managers
