// SPDX-License-Identifier: MIT
// gbfr/managers/photo_manager.hpp
//
// `PhotoManager` has a vftable @ 0x145161d78 and a Steam-specific subclass
// `PhotoManagerSteam` that hosts the `ScreenshotReady_t` / `ScreenshotRequested_t`
// Steam API callbacks.
#pragma once

#include "../object.hpp"
#include "../signatures.hpp"

namespace gbfr::managers {

class PhotoManager : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr std::string_view kSingletonName = "PhotoManager";
    static constexpr Address kVftableRva = signatures::vft::kPhotoManager;
};

class PhotoManagerSteam : public PhotoManager {
public:
    using PhotoManager::PhotoManager;
};

} // namespace gbfr::managers
