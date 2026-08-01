// SPDX-License-Identifier: MIT
// gbfr/main_loop.hpp — `MainLoop` / `AppMainLoop`.
//
// Vftable RVAs for Steam build 24245499:
//   AppMainLoop::vftable @ 0x146135510
//   MainLoop::vftable    @ 0x146135550
//
// AppMainLoop vftable slots:
//   [0] 0x1401a7d50 - scalar deleting dtor
//   [1] 0x140089600 - init / boot
//   [2] 0x1400915a0 - per-frame tick
//   [3] 0x14014cf00 - shutdown / drain
//   [4] 0x1400894a0 - no-op pure-virtual stub
//   [5] 0x1400894a0 - no-op pure-virtual stub
#pragma once

#include "object.hpp"
#include "signatures.hpp"

namespace gbfr {

class MainLoop : public GameObject {
public:
    using GameObject::GameObject;

    static constexpr Address kVftableRva = signatures::vft::kMainLoop;
};

class AppMainLoop : public MainLoop {
public:
    using MainLoop::MainLoop;

    static constexpr Address kVftableRva     = signatures::vft::kAppMainLoop;
    static constexpr Address kBootFuncRva    = signatures::func::kAppMainLoopBoot;
    static constexpr Address kTickFuncRva    = signatures::func::kAppMainLoopTick;
    static constexpr Address kShutdownFuncRva = signatures::func::kAppMainLoopShutdown;

    // Address of the constructor that installs the vftable.
    static constexpr Address kConstructorRva = signatures::func::kAppMainLoopCtor;
};

} // namespace gbfr
