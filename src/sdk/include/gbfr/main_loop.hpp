// SPDX-License-Identifier: MIT
// gbfr/main_loop.hpp — `MainLoop` / `AppMainLoop`.
//
// Vftable RVAs from `vft_AppMainLoop.txt`:
//   AppMainLoop::vftable @ 0x145153708
//   MainLoop::vftable    @ 0x145153748
//
// AppMainLoop vftable slots:
//   [0] 0x140198260 (38 B)     - scalar deleting dtor
//   [1] 0x140079040 (13053 B)  - init / boot   (constructs SaveDataReadModule)
//   [2] 0x14007f460 (328495 B) - per-frame tick (constructs the
//                                 `sys::data::*` cluster on first run)
//   [3] 0x140148360 (2697 B)   - shutdown / drain
//   [4] 0x14007d530 (1 B)      - pure-virtual stub
//   [5] 0x14007d530 (1 B)      - pure-virtual stub
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

    // Address of the constructor that installs the vftable (FUN_140194430).
    static constexpr Address kConstructorRva = signatures::func::kAppMainLoopCtor;
};

} // namespace gbfr
