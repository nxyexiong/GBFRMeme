// SPDX-License-Identifier: MIT
// src/ui.cpp — ImGui draw code for the GBFR UI.
//
// One window per `gbfr_core` C-API section. Section bodies are empty for
// now and will be filled in as the corresponding API surface stabilises.

#include "gbfr/ui/ui.hpp"

#include <imgui.h>

namespace gbfr::ui {

namespace {

void section_character_manager() {
    if (ImGui::CollapsingHeader("CharacterManager", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled("Per-character runtime state (level, exp, equipment)");
        ImGui::TextDisabled("and the 4 party slots (PlayerEquipDataAccumulater[4]).");
        ImGui::Spacing();
        ImGui::TextUnformatted("(no widgets yet)");
    }
}

void section_item_manager() {
    if (ImGui::CollapsingHeader("ItemManager", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled("Consumables / materials inventory and reward generation.");
        ImGui::Spacing();
        ImGui::TextUnformatted("(no widgets yet)");
    }
}

void section_player_data_manager() {
    if (ImGui::CollapsingHeader("PlayerDataManager", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled("Multiplayer player cache (NetworkReadCacheData).");
        ImGui::Spacing();
        ImGui::TextUnformatted("(no widgets yet)");
    }
}

} // namespace

void draw() {
    // Single host window containing one collapsing header per C-API section.
    // Resizable so the user can dock / float as they wish.
    if (ImGui::Begin("GBFRMeme", nullptr, ImGuiWindowFlags_NoCollapse)) {
        section_character_manager();
        section_item_manager();
        section_player_data_manager();
    }
    ImGui::End();
}

} // namespace gbfr::ui
