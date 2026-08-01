// SPDX-License-Identifier: MIT
// gbfr/managers/character_manager.hpp
//
// `CharacterManager` has no virtual methods (no vftable was emitted), so it
// is reached only through the cyan singleton registry. The class string
// The standalone singleton-name string used before game 2.0 is no longer
// emitted; `kNameStringRva` is therefore zero on the current build.
//
// Inner types observed in RTTI:
//   - CharacterManager::QuestUseParam
//   - CharacterManager::ReserveAddExp
//   - CharacterManager::MultiCharaDataCacheType (enum)
//
// Members observed in template instantiations (in `classes.txt`):
//   - unordered_map<string_hash32, unique_ptr<PlayerEquipDataAccumulater>>
//   - array<pair<sys::data::MultiCharaData, PlayerEquipDataAccumulater>, 4>
//     keyed by MultiCharaDataCacheType (party slots)
#pragma once

#include "../object.hpp"
#include "../signatures.hpp"

namespace gbfr::managers {

class PlayerEquipDataAccumulater : public GameObject {
public:
    using GameObject::GameObject;
};

class CharacterManager : public GameObject {
public:
    using GameObject::GameObject;

    static constexpr std::string_view kSingletonName = "CharacterManager";
    static constexpr Address kNameStringRva = signatures::name_string::kCharacterManager;

    // Number of party slots — confirmed by the 4-element `array<...>` in RTTI.
    static constexpr std::size_t kPartySize = signatures::count::kPartySize;

    enum class MultiCharaDataCacheType : std::uint32_t {
        // Exact enumerator values not yet recovered.
        Slot0 = 0, Slot1, Slot2, Slot3,
    };

    static constexpr std::ptrdiff_t kEquipMapOffset =
        GBFR_TODO_OFFSET("CharacterManager: offset of per-character equip map");
    static constexpr std::ptrdiff_t kPartyCacheOffset =
        GBFR_TODO_OFFSET("CharacterManager: offset of per-party-slot cache");

    // Address of the `PlayerEquipDataAccumulater` for a given party slot.
    // Returns 0 if offsets are not yet known.
    [[nodiscard]] PlayerEquipDataAccumulater party_slot(MultiCharaDataCacheType slot) const {
        const auto idx = static_cast<std::size_t>(slot);
        if constexpr (kPartyCacheOffset < 0) {
            (void)idx;
            return {};
        } else {
            if (idx >= kPartySize) return {};
            // TODO: stride is sizeof(pair<MultiCharaData, PlayerEquipDataAccumulater>).
            return PlayerEquipDataAccumulater{}; // placeholder
        }
    }
};

} // namespace gbfr::managers
