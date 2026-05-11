// SPDX-License-Identifier: MIT
// gbfr/game.hpp — top-level facade for the SDK.
//
// Owns an `IMemory` backend and exposes every manager/singleton/main-loop
// wrapper as a typed accessor. Instance addresses must be configured by the
// host before use (the game resolves them via `cyan::Singleton<T>` keyed by
// `string_hash32`; the registry itself is not yet located, so this is a
// runtime concern for the host).
//
// Usage:
//   gbfr::InProcessMemory mem{GetModuleHandleW(L"granblue_fantasy_relink.exe")};
//   gbfr::Game game(mem);
//   game.set_main_loop_address(/* discovered via signature scan */);
//   auto loop = game.main_loop();
//   if (loop.is_valid()) { ... }
#pragma once

#include "cameras/camera.hpp"
#include "common.hpp"
#include "main_loop.hpp"
#include "memory.hpp"
#include "managers/character_manager.hpp"
#include "managers/item_manager.hpp"
#include "managers/network_manager.hpp"
#include "managers/npc_appearance_manager.hpp"
#include "managers/photo_manager.hpp"
#include "managers/player_data_manager.hpp"
#include "managers/quest_systems.hpp"
#include "managers/save_data_manager.hpp"
#include "managers/user_data_manager.hpp"
#include "save/slot_data.hpp"

namespace gbfr {

class Game {
public:
    explicit Game(IMemory& memory) noexcept : m_memory(&memory) {}

    [[nodiscard]] IMemory& memory() const noexcept { return *m_memory; }
    [[nodiscard]] Address  module_base() const noexcept { return m_memory->module_base(); }

    // RVA -> absolute address in the running target.
    [[nodiscard]] Address rva(Address rva_value) const noexcept {
        return rva_to_absolute(module_base(), rva_value);
    }

    // ----- Singleton instance addresses -----------------------------------
    // The host is responsible for discovering these (signature scan or
    // hooking the `cyan::Singleton<T>::getInstance` accessors). Once set,
    // the corresponding accessor returns a valid wrapper.

    void set_main_loop_address(Address a)        noexcept { m_main_loop = a; }
    void set_character_manager_address(Address a) noexcept { m_character_manager = a; }
    void set_item_manager_address(Address a)     noexcept { m_item_manager = a; }
    void set_user_data_manager_address(Address a) noexcept { m_user_data_manager = a; }
    void set_player_data_manager_address(Address a) noexcept { m_player_data_manager = a; }
    void set_save_data_manager_address(Address a) noexcept { m_save_data_manager = a; }
    void set_network_rpc_manager_address(Address a) noexcept { m_network_rpc_manager = a; }
    void set_photo_manager_address(Address a)    noexcept { m_photo_manager = a; }
    void set_npc_appearance_manager_address(Address a) noexcept { m_npc_appearance_manager = a; }
    void set_quest_system_address(Address a)     noexcept { m_quest_system = a; }
    void set_progress_manager_address(Address a) noexcept { m_progress_manager = a; }
    void set_camera_game_address(Address a)      noexcept { m_camera_game = a; }

    // ----- Typed accessors ------------------------------------------------

    [[nodiscard]] AppMainLoop main_loop() const { return {*m_memory, m_main_loop}; }

    [[nodiscard]] managers::CharacterManager   character_manager()    const { return {*m_memory, m_character_manager}; }
    [[nodiscard]] managers::ItemManager        item_manager()         const { return {*m_memory, m_item_manager}; }
    [[nodiscard]] managers::UserDataManager    user_data_manager()    const { return {*m_memory, m_user_data_manager}; }
    [[nodiscard]] managers::PlayerDataManager  player_data_manager()  const { return {*m_memory, m_player_data_manager}; }
    [[nodiscard]] managers::SaveDataManager    save_data_manager()    const { return {*m_memory, m_save_data_manager}; }
    [[nodiscard]] managers::NetworkSystemRpcManager network_rpc_manager() const { return {*m_memory, m_network_rpc_manager}; }
    [[nodiscard]] managers::PhotoManager       photo_manager()        const { return {*m_memory, m_photo_manager}; }
    [[nodiscard]] managers::NpcAppearanceManager npc_appearance_manager() const { return {*m_memory, m_npc_appearance_manager}; }
    [[nodiscard]] managers::quest::QuestSystem quest_system()         const { return {*m_memory, m_quest_system}; }
    [[nodiscard]] managers::quest::ProgressManager progress_manager() const { return {*m_memory, m_progress_manager}; }
    [[nodiscard]] camera::CameraGame           camera_game()          const { return {*m_memory, m_camera_game}; }

    // ----- Slot save data --------------------------------------------------
    //
    // The host must bind each `sys::data::*List` address (e.g. by scanning
    // the parent struct for the known vftable pointers from the RVAs encoded
    // on each wrapper class). Once `binding.cluster_base != 0` and the
    // per-list offsets are filled in, `slot()` returns a fully wired view.
    struct SlotBinding {
        Address chara_list = 0;
        Address chara_preset_list = 0;
        Address weapon_id_save_list = 0;
        Address gem_id_save_list = 0;
        Address gem_list = 0;
        Address item_list = 0;
        Address item_pendulum_list = 0;
        Address ability_list = 0;
        Address scenario_list = 0;
        Address fate_ep_list = 0;
        Address main_story_list = 0;
        Address trade_list = 0;
        Address gacha_list = 0;
        Address island_list = 0;
        Address archive_list = 0;
        Address collectibles_em_list = 0;
        Address collectibles_ba_list = 0;
        Address collectibles_chest_list = 0;
        Address bgm_list = 0;
        Address picture_book_chara_list = 0;
        Address picture_book_enemy_list = 0;
        Address picture_book_pendulum_list = 0;
        Address tips_list = 0;
        Address tutorial_list = 0;
        Address menu_tutorial_list = 0;
        Address menu_unlock_save_data = 0;
        Address infomation_quest_list = 0;
        Address infomation_dialog_list = 0;
        Address command_combo_list = 0;
        Address wordlist_list = 0;
        Address npc_voice_list = 0;
    };

    void set_slot_binding(const SlotBinding& b) noexcept { m_slot_binding = b; }
    [[nodiscard]] save::Slot slot() const;

private:
    IMemory* m_memory{nullptr};

    Address m_main_loop{0};
    Address m_character_manager{0};
    Address m_item_manager{0};
    Address m_user_data_manager{0};
    Address m_player_data_manager{0};
    Address m_save_data_manager{0};
    Address m_network_rpc_manager{0};
    Address m_photo_manager{0};
    Address m_npc_appearance_manager{0};
    Address m_quest_system{0};
    Address m_progress_manager{0};
    Address m_camera_game{0};

    SlotBinding m_slot_binding{};
};

} // namespace gbfr
