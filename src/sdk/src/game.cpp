// SPDX-License-Identifier: MIT
#include "gbfr/game.hpp"

namespace gbfr {

save::Slot Game::slot() const {
    save::Slot s;
    const auto& b = m_slot_binding;
    s.chara_list                 = {*m_memory, b.chara_list};
    s.chara_preset_list          = {*m_memory, b.chara_preset_list};
    s.weapon_id_save_list        = {*m_memory, b.weapon_id_save_list};
    s.gem_id_save_list           = {*m_memory, b.gem_id_save_list};
    s.gem_list                   = {*m_memory, b.gem_list};
    s.item_list                  = {*m_memory, b.item_list};
    s.item_pendulum_list         = {*m_memory, b.item_pendulum_list};
    s.summon_stone_list          = {*m_memory, b.summon_stone_list};
    s.ability_list               = {*m_memory, b.ability_list};
    s.scenario_list              = {*m_memory, b.scenario_list};
    s.fate_ep_list               = {*m_memory, b.fate_ep_list};
    s.main_story_list            = {*m_memory, b.main_story_list};
    s.trade_list                 = {*m_memory, b.trade_list};
    s.gacha_list                 = {*m_memory, b.gacha_list};
    s.island_list                = {*m_memory, b.island_list};
    s.archive_list               = {*m_memory, b.archive_list};
    s.collectibles_em_list       = {*m_memory, b.collectibles_em_list};
    s.collectibles_ba_list       = {*m_memory, b.collectibles_ba_list};
    s.collectibles_chest_list    = {*m_memory, b.collectibles_chest_list};
    s.bgm_list                   = {*m_memory, b.bgm_list};
    s.picture_book_chara_list    = {*m_memory, b.picture_book_chara_list};
    s.picture_book_enemy_list    = {*m_memory, b.picture_book_enemy_list};
    s.picture_book_pendulum_list = {*m_memory, b.picture_book_pendulum_list};
    s.tips_list                  = {*m_memory, b.tips_list};
    s.tutorial_list              = {*m_memory, b.tutorial_list};
    s.menu_tutorial_list         = {*m_memory, b.menu_tutorial_list};
    s.menu_unlock_save_data      = {*m_memory, b.menu_unlock_save_data};
    s.infomation_quest_list      = {*m_memory, b.infomation_quest_list};
    s.infomation_dialog_list     = {*m_memory, b.infomation_dialog_list};
    s.command_combo_list         = {*m_memory, b.command_combo_list};
    s.wordlist_list              = {*m_memory, b.wordlist_list};
    s.npc_voice_list             = {*m_memory, b.npc_voice_list};
    return s;
}

} // namespace gbfr
