// SPDX-License-Identifier: MIT
// gbfr/save/data_rows.hpp — POD structs that mirror `sys::data::*Data` save rows.
//
// Layouts are NOT recovered yet. Each struct is sized as a fixed buffer with
// named field accessors that return `GBFR_TODO_OFFSET` -- they will return
// garbage until the offsets are filled in. The structs themselves are kept
// `std::byte` arrays so the templates in `save_list_base.hpp` can be
// instantiated.
//
// To resolve a field, decompile the `ss::reflection::ObjectType<...>::createAttr`
// builder for that data type (every reflected type has one). The `Attribute`
// constructor records (member offset, type) tuples — those are the field
// layouts.
#pragma once

#include "../common.hpp"

#include <array>
#include <cstring>

namespace gbfr::save::row {

// Helper: typed accessor inside a POD blob.
template <std::size_t Size>
struct RowBlob {
    std::array<std::byte, Size> bytes{};

    template <class T>
    [[nodiscard]] T field(std::ptrdiff_t offset) const noexcept {
        T v{};
        std::memcpy(&v, bytes.data() + offset, sizeof(T));
        return v;
    }
};

// Sizes below are MINIMUM-PLAUSIBLE guesses (large enough to hold the
// observed entry capacities at sane field widths). They MUST be confirmed
// against actual `sizeof(T)` in the binary before relying on them.

struct CharaData              : RowBlob<0x200> {};  // CharaList(40)
struct CharaPresetCharaData   : RowBlob<0x200> {};  // CharaPresetList(40)
struct WeaponIdSave           : RowBlob<0x40>  {};  // WeaponIdSaveList(512)
struct GemIdSave              : RowBlob<0x40>  {};  // GemIdSaveList(900)
struct GemData                : RowBlob<0x40>  {};  // GemList(5100)
struct ItemData               : RowBlob<0x40>  {};  // ItemList(300)
struct ItemPendulum           : RowBlob<0x40>  {};  // ItemPendulumList(5000)
struct AbilityData            : RowBlob<0x20>  {};  // AbilityList(640)
struct ScenarioData           : RowBlob<0x40>  {};  // ScenarioList(64)
struct FateData               : RowBlob<0x40>  {};  // FateEpList(800)
struct IslandData             : RowBlob<0x40>  {};  // IslandList(32)
struct GachaData              : RowBlob<0x40>  {};  // GachaList(10)
struct ArchiveData            : RowBlob<0x40>  {};  // ArchiveList(100)
struct CollectiblesEmData     : RowBlob<0x20>  {};  // CollectiblesEmList(256)
struct CollectiblesBaData     : RowBlob<0x20>  {};  // CollectiblesBaList(256)
struct CollectiblesChestData  : RowBlob<0x20>  {};  // CollectiblesChestList(256)
struct TradeData              : RowBlob<0x40>  {};  // TradeList(1024)
struct MainStoryData          : RowBlob<0x40>  {};  // MainStoryList(260)
struct BGMData                : RowBlob<0x20>  {};  // BGMList(100)
struct PictureBookCharaData   : RowBlob<0x20>  {};  // PictureBookCharaList(50)
struct PictureBookEnemyData   : RowBlob<0x20>  {};  // PictureBookEnemyList(150)
struct PictureBookPendulumData: RowBlob<0x20>  {};  // PictureBookPendulumList(10)
struct TipsData               : RowBlob<0x20>  {};  // TipsList(500)
struct TutorialWindowData     : RowBlob<0x20>  {};  // TutorialList(300)
struct TutorialMenuData       : RowBlob<0x20>  {};  // MenuTutorialList(300)
struct MenuUnlockSaveOne      : RowBlob<0x20>  {};  // MenuUnlockSaveData(128)
struct InfomationQuestData    : RowBlob<0x40>  {};  // InfomationQuestList(64)
struct InfomationDialogData   : RowBlob<0x40>  {};  // InfomationDialogList(64)
struct CommandComboData       : RowBlob<0x40>  {};  // CommandComboList(800)
struct WordlistData           : RowBlob<0x20>  {};  // WordlistList(150)
struct NpcVoiceData           : RowBlob<0x20>  {};  // NpcVoiceList(32)

} // namespace gbfr::save::row
