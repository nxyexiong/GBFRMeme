// SPDX-License-Identifier: MIT
// gbfr/ui/host.hpp — Win32 + D3D11 window that runs the ImGui UI.
//
// Two intended deployments:
//
//   * External tool       — a fixed-size top-level window. Closes via the
//                           usual X button. No hotkey.
//   * In-process overlay  — an always-on-top, initially-hidden window
//                           toggled by a global hotkey (e.g. VK_F1).
//
// The host owns its own message loop. Call `run()` from a worker thread,
// then `request_stop()` from any thread to shut down.
#pragma once

#include <functional>
#include <memory>
#include <string>

namespace gbfr::ui {

struct HostOptions {
    int          width        = 960;
    int          height       = 640;
    bool         topmost      = false;          // WS_EX_TOPMOST
    bool         start_hidden = false;          // initial ShowWindow(SW_HIDE)
    int          hotkey_vk    = 0;              // 0 = no hotkey; else virtual-key
    unsigned     hotkey_mods  = 0;              // MOD_ALT / MOD_CONTROL / MOD_SHIFT / MOD_WIN
    std::wstring title        = L"GBFRMeme";
};

// Callback invoked once per frame between ImGui::NewFrame and Render.
using DrawCallback = std::function<void()>;

class Host {
public:
    Host(const HostOptions& opts, DrawCallback draw);
    ~Host();

    Host(const Host&)            = delete;
    Host& operator=(const Host&) = delete;
    Host(Host&&)                 = delete;
    Host& operator=(Host&&)      = delete;

    // Blocking message loop. Returns when the window is destroyed or
    // `request_stop()` was called. Returns false if initialization failed.
    bool run();

    // Thread-safe; posts WM_QUIT to the host thread.
    void request_stop();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace gbfr::ui
