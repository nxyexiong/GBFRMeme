// SPDX-License-Identifier: MIT
// gbfr/managers/user_data_manager.hpp
//
// `UserDataManager` — front for Steam / platform user data. No vftable.
// Name string at `s_UserDataManager` (RVA 0x1451b1d7a).
// Observed methods (from lambda RTTI):
//   - bool UserDataManager::requestDLCInfo();
#pragma once

#include "../object.hpp"

namespace gbfr::managers {

class UserDataManager : public GameObject {
public:
    using GameObject::GameObject;

    static constexpr std::string_view kSingletonName = "UserDataManager";
    static constexpr Address kNameStringRva = 0x1451b1d7aULL;
};

} // namespace gbfr::managers
