// SPDX-License-Identifier: MIT
// gbfr/ui/ui.hpp — top-level UI draw entry point.
//
// Renders the sections that mirror the `gbfr_core` C API (CharacterManager,
// ItemManager, PlayerDataManager). Section bodies are placeholders; widgets
// will be added once the corresponding C-API surface lands.
#pragma once

namespace gbfr::ui {

// Emit ImGui windows / widgets for the current frame. Must be called between
// `ImGui::NewFrame()` and `ImGui::Render()` — the `Host` runs it inside its
// frame for you.
void draw();

} // namespace gbfr::ui
