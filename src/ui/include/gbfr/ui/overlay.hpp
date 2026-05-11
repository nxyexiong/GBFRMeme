// SPDX-License-Identifier: MIT
// gbfr/ui/overlay.hpp — in-process ImGui overlay drawn into the host
// application's own swap chain.
//
// Mechanism:
//   1. Create a throwaway D3D11 device + swap chain, read its
//      IDXGISwapChain vtable, snapshot the original `Present` /
//      `ResizeBuffers` pointers, then swap the vtable entries with our
//      trampolines. The dummy is released immediately afterwards.
//   2. On the first hooked `Present`, capture the game's device, context
//      and HWND from the live swap chain, initialise ImGui's Win32/DX11
//      backends, and subclass the HWND so input messages reach ImGui.
//   3. Each subsequent `Present` runs `NewFrame -> draw() -> Render ->
//      RenderDrawData` against the game's render target before the
//      original `Present` is called.
//   4. While the overlay is visible, mouse / keyboard messages that ImGui
//      wants to capture are swallowed from the game's WNDPROC.
//
// Visibility is toggled by polling a virtual-key via `GetAsyncKeyState`
// inside the `Present` hook (e.g. VK_F1).
#pragma once

#include "gbfr/ui/host.hpp"   // DrawCallback

namespace gbfr::ui {

struct OverlayOptions {
    int  hotkey_vk     = 0;     // 0 = no hotkey; e.g. VK_F1
    bool start_visible = false;
};

class Overlay {
public:
    Overlay(const OverlayOptions& opts, DrawCallback draw);
    ~Overlay();

    Overlay(const Overlay&)            = delete;
    Overlay& operator=(const Overlay&) = delete;
    Overlay(Overlay&&)                 = delete;
    Overlay& operator=(Overlay&&)      = delete;

    // Patch the IDXGISwapChain vtable. Returns false if the dummy device
    // / swap chain could not be created (e.g. no D3D11-capable adapter).
    [[nodiscard]] bool install();

    // Restore the vtable and tear down ImGui state.
    void uninstall();
};

} // namespace gbfr::ui
