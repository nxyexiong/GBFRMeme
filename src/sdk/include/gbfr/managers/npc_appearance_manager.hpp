// SPDX-License-Identifier: MIT
// gbfr/managers/npc_appearance_manager.hpp
//
// `cNpcAppearanceManager` — vftable @ 0x1447fe390.
// Owns `cAppearsNpcHandle` / `cAppearanceNpc` runtime objects.
#pragma once

#include "../object.hpp"
#include "../signatures.hpp"

namespace gbfr::managers {

class NpcAppearanceManager : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr std::string_view kSingletonName = "cNpcAppearanceManager";
    static constexpr Address kVftableRva = signatures::vft::kNpcAppearanceManager;
};

} // namespace gbfr::managers
