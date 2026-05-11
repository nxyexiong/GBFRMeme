// SPDX-License-Identifier: MIT
// gbfr/cameras/camera.hpp — camera hierarchy.
//
// Inheritance (from RTTI):
//   Fw::cCameraBase -> cCameraApp -> cCameraGame
//
// cCameraGame::vftable @ 0x1447fda38 has 8 entries.
#pragma once

#include "../object.hpp"

namespace gbfr::camera {

class CameraBase : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = 0x14480b6d0ULL; // Fw::cCameraBase::vftable
};

class CameraApp : public CameraBase {
public:
    using CameraBase::CameraBase;
    static constexpr Address kVftableRva = 0x1447fdb88ULL;
};

class CameraGame : public CameraApp {
public:
    using CameraApp::CameraApp;
    static constexpr Address kVftableRva = 0x1447fda38ULL;

    // Mode classes (one vftable each, address taken from vftables_index.tsv):
    static constexpr Address kVftBattleCutsceneRva = 0x1447fdef8ULL;
};

} // namespace gbfr::camera
