// SPDX-License-Identifier: MIT
// gbfr/ui/ui.hpp — top-level UI draw entry point.
//
// Renders wallet, item, sigil, wrightstone, and summon editors backed by the
// `gbfr_core` C API.
#pragma once

namespace gbfr::ui {

// Emit ImGui windows / widgets for the current frame. Must be called between
// `ImGui::NewFrame()` and `ImGui::Render()` — the `Host` runs it inside its
// frame for you.
void draw();

} // namespace gbfr::ui
