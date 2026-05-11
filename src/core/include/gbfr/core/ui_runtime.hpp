// SPDX-License-Identifier: MIT
// gbfr/core/ui_runtime.hpp — owns the worker thread that hosts the UI.
//
// `gbfr_init` creates a `UiRuntime`; `gbfr_shutdown` destroys it. The
// runtime starts an OS thread and runs the `ui::Host` on it so the host
// process (or game) keeps its own main thread free.
#pragma once

#include "gbfr/core/c_api.h"

#include <memory>

namespace gbfr::core {

class UiRuntime {
public:
    UiRuntime();
    ~UiRuntime();

    UiRuntime(const UiRuntime&)            = delete;
    UiRuntime& operator=(const UiRuntime&) = delete;
    UiRuntime(UiRuntime&&)                 = delete;
    UiRuntime& operator=(UiRuntime&&)      = delete;

    // Spawn the UI thread. Window style and hotkey are chosen based on
    // attach mode:
    //   * EXTERNAL → fixed-size top-level window, no hotkey.
    //   * INTERNAL → always-on-top overlay, initially hidden, toggled by F1.
    void start(GbfrAttachMode mode);

    // Block the calling thread until the host window thread exits.
    // No-op for INTERNAL mode or when no host thread is running.
    void wait_for_exit();

    // Signal the host to exit and wait (bounded) for the thread to leave.
    // If the wait times out the thread is detached.
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace gbfr::core
