// SPDX-License-Identifier: MIT
// gbfr/managers/user_data_manager.hpp
//
// `UserDataManager` — front for Steam / platform user data. No vftable.
// The pre-2.0 standalone singleton-name string is no longer emitted.
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
