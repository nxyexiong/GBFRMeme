// SPDX-License-Identifier: MIT
// gbfr/cameras/camera.hpp — camera hierarchy.
//
// Inheritance (from RTTI):
//   Fw::cCameraBase -> cCameraApp -> cCameraGame
//
// cCameraGame::vftable @ 0x1447fda38 has 8 entries.
#pragma once

#include "../object.hpp"
#include "../signatures.hpp"

namespace gbfr::camera {

class CameraBase : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = signatures::vft::kCameraBase;
};

class CameraApp : public CameraBase {
public:
    using CameraBase::CameraBase;
    static constexpr Address kVftableRva = signatures::vft::kCameraApp;
};

class CameraGame : public CameraApp {
public:
    using CameraApp::CameraApp;
    static constexpr Address kVftableRva = signatures::vft::kCameraGame;

    // Mode classes (one vftable each):
    static constexpr Address kVftBattleCutsceneRva = signatures::vft::kCameraBattleCutscene;
};

} // namespace gbfr::camera
