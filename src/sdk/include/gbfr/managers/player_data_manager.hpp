// SPDX-License-Identifier: MIT
// gbfr/managers/player_data_manager.hpp
//
// `PlayerDataManager`. No vftable. Inner type observed:
//   - PlayerDataManager::NetworkReadCacheData
#pragma once

#include "../object.hpp"

namespace gbfr::managers {

class PlayerDataManager : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr std::string_view kSingletonName = "PlayerDataManager";
};

} // namespace gbfr::managers
