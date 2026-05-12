// SPDX-License-Identifier: MIT
// gbfr/managers/user_data_manager.hpp
//
// `UserDataManager` — front for Steam / platform user data. No vftable.
// Name string at `s_UserDataManager` (RVA 0x1451b1d7a).
// Observed methods (from lambda RTTI):
//   - bool UserDataManager::requestDLCInfo();
#pragma once

#include "../object.hpp"
#include "../signatures.hpp"

namespace gbfr::managers {

class UserDataManager : public GameObject {
public:
    using GameObject::GameObject;

    static constexpr std::string_view kSingletonName = "UserDataManager";
    static constexpr Address kNameStringRva = signatures::name_string::kUserDataManager;
};

} // namespace gbfr::managers
