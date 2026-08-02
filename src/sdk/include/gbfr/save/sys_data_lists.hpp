// SPDX-License-Identifier: MIT
// gbfr/save/sys_data_lists.hpp — catalogue of `sys::data::*List` save-data containers.
//
// Each wrapper records:
//   - kVftableRva : RVA of `sys::data::Xxx::vftable` (see signatures::vft).
//   - Type alias  : the underlying `SaveListBase<TData, Key, Max>` /
//                   `SaveTempListBase<TData, Key, Max, Chunk>` template.
//
// All RVAs live in `gbfr/signatures.hpp`; this file is structural only.
#pragma once

#include "../signatures.hpp"
#include "data_rows.hpp"
#include "save_list_base.hpp"

namespace gbfr::sys::data {

// ----- table::SaveListBase derivatives ------------------------------------

class ScenarioList            : public save::SaveListBase<save::row::ScenarioData,            StringHash32,  64> {
public:
    static constexpr Address kVftableRva = signatures::vft::kScenarioList;
    using SaveListBase::SaveListBase;
};

class FateEpList              : public save::SaveListBase<save::row::FateData,                StringHash32, 800> {
public:
    static constexpr Address kVftableRva = signatures::vft::kFateEpList;
    using SaveListBase::SaveListBase;
};

class IslandList              : public save::SaveListBase<save::row::IslandData,              StringHash32,  32> {
public:
    static constexpr Address kVftableRva = signatures::vft::kIslandList;
    using SaveListBase::SaveListBase;
};

class GachaList               : public save::SaveListBase<save::row::GachaData,               StringHash32,  10> {
public:
    static constexpr Address kVftableRva = signatures::vft::kGachaList;
    using SaveListBase::SaveListBase;
};

class ArchiveList             : public save::SaveListBase<save::row::ArchiveData,             StringHash32, 100> {
public:
    static constexpr Address kVftableRva = signatures::vft::kArchiveList;
    using SaveListBase::SaveListBase;
};

class CollectiblesEmList      : public save::SaveListBase<save::row::CollectiblesEmData,      std::uint32_t, 256> {
public:
    static constexpr Address kVftableRva = signatures::vft::kCollectiblesEmList;
    using SaveListBase::SaveListBase;
};

class CollectiblesBaList      : public save::SaveListBase<save::row::CollectiblesBaData,      std::uint32_t, 256> {
public:
    static constexpr Address kVftableRva = signatures::vft::kCollectiblesBaList;
    using SaveListBase::SaveListBase;
};

class CollectiblesChestList   : public save::SaveListBase<save::row::CollectiblesChestData,   StringHash32, 256> {
public:
    static constexpr Address kVftableRva = signatures::vft::kCollectiblesChestList;
    using SaveListBase::SaveListBase;
};

class AbilityList             : public save::SaveListBase<save::row::AbilityData,             StringHash32, 640> {
public:
    static constexpr Address kVftableRva = signatures::vft::kAbilityList;
    using SaveListBase::SaveListBase;
};

class TradeList               : public save::SaveListBase<save::row::TradeData,               StringHash32,1024> {
public:
    static constexpr Address kVftableRva = signatures::vft::kTradeList;
    using SaveListBase::SaveListBase;
};

class MainStoryList           : public save::SaveListBase<save::row::MainStoryData,           StringHash32, 260> {
public:
    static constexpr Address kVftableRva = signatures::vft::kMainStoryList;
    using SaveListBase::SaveListBase;
};

class BGMList                 : public save::SaveListBase<save::row::BGMData,                 StringHash32, 100> {
public:
    static constexpr Address kVftableRva = signatures::vft::kBGMList;
    using SaveListBase::SaveListBase;
};

class PictureBookCharaList    : public save::SaveListBase<save::row::PictureBookCharaData,    StringHash32,  50> {
public:
    static constexpr Address kVftableRva = signatures::vft::kPictureBookCharaList;
    using SaveListBase::SaveListBase;
};

class PictureBookEnemyList    : public save::SaveListBase<save::row::PictureBookEnemyData,    StringHash32, 150> {
public:
    static constexpr Address kVftableRva = signatures::vft::kPictureBookEnemyList;
    using SaveListBase::SaveListBase;
};

class PictureBookPendulumList : public save::SaveListBase<save::row::PictureBookPendulumData, StringHash32,  10> {
public:
    static constexpr Address kVftableRva = signatures::vft::kPictureBookPendulumList;
    using SaveListBase::SaveListBase;
};

class TipsList                : public save::SaveListBase<save::row::TipsData,                StringHash32, 500> {
public:
    static constexpr Address kVftableRva = signatures::vft::kTipsList;
    using SaveListBase::SaveListBase;
};

class InfomationQuestList     : public save::SaveListBase<save::row::InfomationQuestData,     std::uint32_t, 64> {
public:
    static constexpr Address kVftableRva = signatures::vft::kInfomationQuestList;
    using SaveListBase::SaveListBase;
};

class InfomationDialogList    : public save::SaveListBase<save::row::InfomationDialogData,    StringHash32,  64> {
public:
    static constexpr Address kVftableRva = signatures::vft::kInfomationDialogList;
    using SaveListBase::SaveListBase;
};

class CharaList               : public save::SaveListBase<save::row::CharaData,               StringHash32,  40, 0x3120> {
public:
    static constexpr Address kVftableRva = signatures::vft::kCharaList;
    using SaveListBase::SaveListBase;
};

class CharaPresetList         : public save::SaveListBase<save::row::CharaPresetCharaData,    StringHash32,  40> {
public:
    static constexpr Address kVftableRva = signatures::vft::kCharaPresetList;
    using SaveListBase::SaveListBase;
};

class WeaponIdSaveList        : public save::SaveListBase<save::row::WeaponIdSave,            StringHash32, 512> {
public:
    static constexpr Address kVftableRva = signatures::vft::kWeaponIdSaveList;
    using SaveListBase::SaveListBase;
};

class GemIdSaveList           : public save::SaveListBase<save::row::GemIdSave,               StringHash32, 900> {
public:
    static constexpr Address kVftableRva = signatures::vft::kGemIdSaveList;
    using SaveListBase::SaveListBase;
};

class ItemList                : public save::SaveListBase<save::row::ItemData,                StringHash32, 300> {
public:
    static constexpr Address kVftableRva = signatures::vft::kItemList;
    using SaveListBase::SaveListBase;
};

class WordlistList            : public save::SaveListBase<save::row::WordlistData,            StringHash32, 150> {
public:
    static constexpr Address kVftableRva = signatures::vft::kWordlistList;
    using SaveListBase::SaveListBase;
};

class MenuUnlockSaveData      : public save::SaveListBase<save::row::MenuUnlockSaveOne,       StringHash32, 128> {
public:
    static constexpr Address kVftableRva = signatures::vft::kMenuUnlockSaveData;
    using SaveListBase::SaveListBase;
};

class TutorialList            : public save::SaveListBase<save::row::TutorialWindowData,      StringHash32, 300> {
public:
    static constexpr Address kVftableRva = signatures::vft::kTutorialList;
    using SaveListBase::SaveListBase;
};

class MenuTutorialList        : public save::SaveListBase<save::row::TutorialMenuData,        StringHash32, 300> {
public:
    static constexpr Address kVftableRva = signatures::vft::kMenuTutorialList;
    using SaveListBase::SaveListBase;
};

class NpcVoiceList            : public save::SaveListBase<save::row::NpcVoiceData,            StringHash32,  32> {
public:
    static constexpr Address kVftableRva = signatures::vft::kNpcVoiceList;
    using SaveListBase::SaveListBase;
};

class CommandComboList        : public save::SaveListBase<save::row::CommandComboData,        StringHash32, 800> {
public:
    static constexpr Address kVftableRva = signatures::vft::kCommandComboList;
    using SaveListBase::SaveListBase;
};

// ----- table::SaveTempListBase derivatives --------------------------------

class GemList                 : public save::SaveTempListBase<save::row::GemData,         std::uint32_t, 5100,  999> {
public:
    static constexpr Address kVftableRva = signatures::vft::kGemList;
    using SaveTempListBase::SaveTempListBase;
};

class ItemPendulumList        : public save::SaveTempListBase<save::row::ItemPendulum,    std::uint32_t, 5000, 5000> {
public:
    static constexpr Address kVftableRva = signatures::vft::kItemPendulumList;
    using SaveTempListBase::SaveTempListBase;
};

class SummonStoneList         : public save::SaveTempListBase<save::row::SummonStoneData, std::uint32_t, 1000,  999> {
public:
    static constexpr Address kVftableRva = signatures::vft::kSummonStoneList;
    using SaveTempListBase::SaveTempListBase;
};

} // namespace gbfr::sys::data
