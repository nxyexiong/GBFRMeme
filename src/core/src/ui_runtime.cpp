// SPDX-License-Identifier: MIT
#include "gbfr/core/ui_runtime.hpp"

#include "gbfr/ui/host.hpp"
#include "gbfr/ui/overlay.hpp"
#include "gbfr/ui/ui.hpp"

#include <windows.h>

#include <atomic>
#include <memory>
#include <thread>

namespace gbfr::core {

struct UiRuntime::Impl {
    // EXTERNAL path: own a top-level window on a worker thread.
    std::unique_ptr<gbfr::ui::Host>  host;
    std::atomic<gbfr::ui::Host*>     host_raw{nullptr};
    std::thread                      thread;

    // INTERNAL path: hook the game's swap chain and render into it.
    std::unique_ptr<gbfr::ui::Overlay> overlay;
};

UiRuntime::UiRuntime()  : m_impl(std::make_unique<Impl>()) {}
UiRuntime::~UiRuntime() { stop(); }

void UiRuntime::start(GbfrAttachMode mode) {
    if (!m_impl) return;

    if (mode == GBFR_ATTACH_INTERNAL) {
        if (m_impl->overlay) return;

        gbfr::ui::OverlayOptions opts;
        opts.hotkey_vk     = VK_F1;
        opts.start_visible = false;

        auto overlay = std::make_unique<gbfr::ui::Overlay>(
            opts, []{ gbfr::ui::draw(); });

        if (!overlay->install()) {
            // Could not acquire a swap-chain vtable (e.g. no D3D11 adapter
            // available right now). Drop the object; nothing to clean up.
            return;
        }
        m_impl->overlay = std::move(overlay);
        return;
    }

    // EXTERNAL: spin up a worker thread that owns the host window.
    if (m_impl->thread.joinable()) return;

    gbfr::ui::HostOptions opts;
    opts.width        = 1024;
    opts.height       = 768;
    opts.topmost      = false;
    opts.start_hidden = false;
    opts.hotkey_vk    = 0;
    opts.title        = L"GBFRMeme";

    auto host = std::make_unique<gbfr::ui::Host>(
        opts, []{ gbfr::ui::draw(); });

    m_impl->host_raw.store(host.get(), std::memory_order_release);

    auto* raw_slot = &m_impl->host_raw;
    m_impl->thread = std::thread(
        [h = std::move(host), raw_slot]() mutable {
            h->run();
            raw_slot->store(nullptr, std::memory_order_release);
            // `h` is destroyed on the UI thread, so Win32 / DX11 / ImGui
            // teardown happens on the same thread that created them.
        });
}

void UiRuntime::wait_for_exit() {
    if (!m_impl) return;
    if (m_impl->thread.joinable()) {
        m_impl->thread.join();
        m_impl->host_raw.store(nullptr, std::memory_order_release);
    }
}

void UiRuntime::stop() {
    if (!m_impl) return;

    // INTERNAL: unhook the overlay synchronously (vtable patch reverted,
    // brief grace period for in-flight Present calls, then ImGui teardown).
    if (m_impl->overlay) {
        m_impl->overlay->uninstall();
        m_impl->overlay.reset();
    }

    // EXTERNAL: ask the host thread to stop and bounded-wait for it.
    if (auto* h = m_impl->host_raw.load(std::memory_order_acquire)) {
        h->request_stop();
    }
    if (m_impl->thread.joinable()) {
        const HANDLE native = m_impl->thread.native_handle();
        const DWORD  result = WaitForSingleObject(native, 5000);
        if (result == WAIT_OBJECT_0) {
            m_impl->thread.join();
        } else {
            m_impl->thread.detach();
        }
    }
}

} // namespace gbfr::core
