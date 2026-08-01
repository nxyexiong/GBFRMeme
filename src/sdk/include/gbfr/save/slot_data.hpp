// SPDX-License-Identifier: MIT
// gbfr/save/slot_data.hpp — in-memory mirror of slot save data.
//
// The save-data lists are constructed by `AppMainLoop::tick` (slot 2). The
// game 2.0 recompile no longer keeps every list in one contiguous vftable
// cluster. The relative
// member offsets inside the parent struct are NOT yet known — this wrapper
// requires the caller to supply the address of each list independently, OR
// the base address of the cluster plus per-member offsets (TODO).
#pragma once

#include "sys_data_lists.hpp"

namespace gbfr::save {

// A bag of wrappers, one per `sys::data::*List`. Either populated by hand
// (caller fills addresses) or via `Slot::bind_from_cluster_base()` once the
// internal offsets are recovered.
struct Slot {
    sys::data::CharaList                chara_list;
    sys::data::CharaPresetList          chara_preset_list;
    sys::data::WeaponIdSaveList         weapon_id_save_list;
    sys::data::GemIdSaveList            gem_id_save_list;
    sys::data::GemList                  gem_list;
    sys::data::ItemList                 item_list;
    sys::data::ItemPendulumList         item_pendulum_list;
    sys::data::AbilityList              ability_list;
    sys::data::ScenarioList             scenario_list;
    sys::data::FateEpList               fate_ep_list;
    sys::data::MainStoryList            main_story_list;
    sys::data::TradeList                trade_list;
    sys::data::GachaList                gacha_list;
    sys::data::IslandList               island_list;
    sys::data::ArchiveList              archive_list;
    sys::data::CollectiblesEmList       collectibles_em_list;
    sys::data::CollectiblesBaList       collectibles_ba_list;
    sys::data::CollectiblesChestList    collectibles_chest_list;
    sys::data::BGMList                  bgm_list;
    sys::data::PictureBookCharaList     picture_book_chara_list;
    sys::data::PictureBookEnemyList     picture_book_enemy_list;
    sys::data::PictureBookPendulumList  picture_book_pendulum_list;
    sys::data::TipsList                 tips_list;
    sys::data::TutorialList             tutorial_list;
    sys::data::MenuTutorialList         menu_tutorial_list;
    sys::data::MenuUnlockSaveData       menu_unlock_save_data;
    sys::data::InfomationQuestList      infomation_quest_list;
    sys::data::InfomationDialogList     infomation_dialog_list;
    sys::data::CommandComboList         command_combo_list;
    sys::data::WordlistList             wordlist_list;
    sys::data::NpcVoiceList             npc_voice_list;
};

} // namespace gbfr::save
