// SPDX-License-Identifier: MIT
// gbfr/signatures.hpp — single source of truth for all reverse-engineered
// constants the SDK encodes.
//
// Everything that came from static analysis lives here:
//   * vftable RVAs                              (signatures::vft)
//   * class-name strings in .rdata              (signatures::name_string)
//   * known function RVAs                       (signatures::func)
//   * fixed array / list sizes                  (signatures::count)
//   * empty-key / "no entry" sentinels          (signatures::sentinel)
//   * IDA-style byte patterns                   (signatures::pattern)
//   * static instance / pointer slot RVAs       (signatures::instance / ::ptr_slot)
//   * verified field offsets                    (signatures::offset)
//
// All addresses are recovered against the design-time image base
// `0x140000000` (Ghidra default load). Translate to a live address with
// `rva_to_absolute(module_base, rva)` from `common.hpp`.
//
// Field byte offsets that have NOT been verified at runtime stay marked
// `GBFR_TODO_OFFSET(...)` directly on the wrapper class that uses them, so
// the macro and its placeholder value are visible at the call site. Once
// verified, an offset migrates here under `signatures::offset::<scope>::*`.
//
// =========================================================================
//  HOW TO UPDATE THIS FILE WHEN THE GAME PATCHES
// =========================================================================
//
// Every constant in this file is keyed on the Ghidra static analysis of one
// specific build of `granblue_fantasy_relink.exe`. After each game patch,
// most of these RVAs *drift* (functions move, vtables shift, .data layout
// changes). The byte-pattern entries under `signatures::pattern` and the
// per-record `signatures::offset` values are usually more stable, but they
// still need to be re-verified.
//
// The recovery workflow we use lives under `reverse/` and is built on
// Ghidra headless scripting + the SDK's own debug CLI. The general loop:
//
// 1. Refresh the Ghidra project.
//    * Open `reverse/20260510/` (or create a sibling folder for the new
//      build) and re-import `granblue_fantasy_relink.exe`. Let Ghidra run
//      its analyzers to completion.
//    * Re-run the dump scripts in `reverse/scripts/` to refresh the
//      reference TSVs:
//        ExportProgramSummary.java   -> classes.txt, functions.tsv, exports.txt
//        DumpVftables.java           -> all vtable layouts
//        FindManagerSlots.java       -> manager_static_instances.tsv +
//                                       manager_ptr_slots.tsv
//        FindPlayerEntityVftables.java -> per-character vtable RVAs
//          (consumed by `gbfr/character_types.hpp`, NOT this file)
//
// 2. Re-derive each section of this file from those outputs:
//
//    (a) `signatures::vft::*`
//        Search Ghidra symbols for `<Class>::vftable` and copy the address.
//        The 26 character vftables live in `character_types.hpp`, not here.
//        SaveListBase / SaveTempListBase derivatives (CharaList, ItemList,
//        GemList, ItemPendulumList, ...) appear in a contiguous RVA cluster
//        under the save aggregate ctor — easy to spot in classes.txt.
//
//    (b) `signatures::name_string::*`
//        Search `.rdata` for the exact ASCII class-name string used by
//        `cyan::Singleton<T>` lookups (e.g. "CharacterManager",
//        "UserDataManager"). Use `SearchSymbols.java` or grep `classes.txt`.
//
//    (c) `signatures::func::*`
//        Cross-reference AppMainLoop's vtable (vft::kAppMainLoop) and copy
//        the slot[1..3] function RVAs. The ctor address is recovered by
//        finding the function that LEA-installs AppMainLoop::vftable
//        (XREFs to vft::kAppMainLoop). `IdentifyVftables.java` prints
//        these directly.
//
//    (d) `signatures::count::*`
//        Slot caps come from the IMUL instructions inside the list ctor
//        (e.g. `IMUL R14, R12, 0x3120` for CharaList stride; the explicit
//        upper bound comparison gives the count). For ItemList / GemList /
//        ItemPendulumList look at FUN_14007f460 (AppMainLoop tick) and the
//        list-specific ctors it calls. The party size is fixed by the RTTI
//        `array<..., 4>` template arg.
//
//    (e) `signatures::sentinel::kEmptyKey`
//        Run `GBFRMeme.exe -c debug hash ""` against the new build. Should
//        still print `0x887AE0B0` as long as the custom xxh32 seed constants
//        (in `common.hpp`) are unchanged. If the value differs, the engine
//        rotated its hash seeds and `common.hpp` itself needs an update
//        first (see the `.rdata` SIMD init vector recovery instructions
//        in `reverse/20260510/docs/scene-and-save-structure.md`).
//
//    (f) `signatures::pattern::*`
//        Open the function that LEAs the PlayerStats pointer (search for
//        the prior site or rediscover via XREFs to `find_local_player`'s
//        Entity vtable). Re-extract the surrounding 23 instruction bytes
//        and update the IDA-style mask; verify by running the SDK CLI:
//          `GBFRMeme.exe -c debug scan-pattern "<new pattern>"`
//        It should match exactly once. Update `kPlayerDataOffsetDispPos`
//        if the disp32 moved within the pattern.
//
//    (g) `signatures::instance::*` and `signatures::ptr_slot::*`
//        Drive from `manager_static_instances.tsv` /
//        `manager_ptr_slots.tsv`. Each entry there has the vftable RVA the
//        SDK references AND the .data RVA the script discovered, so cross-
//        reference the SDK constant by class name. The `debug instances`
//        CLI subcommand reads every entry and reports whether the
//        dereferenced object's vftable still matches the SDK's expected
//        vft RVA — easy regression check.
//
//    (h) `signatures::offset::*`
//        These are field-byte-offsets inside specific records. Strategies:
//          * save_list_base / chara_data: re-read the per-entry loop in
//            FUN_14007f460. Look for the liveness-sentinel check and the
//            key load.
//          * save_aggregate: search XREFs to the per-list vftables and
//            note the constant offsets that produce the list pointer.
//          * save_data_unit: vftable layout is described in
//            `IdentifyVftables.java` output for SaveDataUnit<int,1>.
//          * user_save_block: see the "Currency / wallet" section of
//            `reverse/20260510/docs/scene-and-save-structure.md`. Verify
//            live by reading the rupie / mastery values after the SDK's
//            `find_user_save_block` scan: `GBFRMeme.exe -c currency`.
//          * item_list / gem_list / item_pendulum_list: stride is
//            `sizeof(EntryRaw)` (static_assert in c_api.cpp catches drift);
//            entries-offset is the constant added to the list pointer
//            before the indexed load in the corresponding accessor.
//
// 3. Sanity-check against the live game.
//    After updating, run the SDK CLI in `external` attach mode:
//      `GBFRMeme.exe -c ping`                  -> module attach OK
//      `GBFRMeme.exe -c debug instances`       -> all anchors resolve
//      `GBFRMeme.exe -c characters`            -> CharaList walks
//      `GBFRMeme.exe -c items`                 -> ItemList walks
//      `GBFRMeme.exe -c sigils`                -> GemList walks
//      `GBFRMeme.exe -c wrightstones`          -> ItemPendulumList walks
//      `GBFRMeme.exe -c currency`              -> wallet discovery succeeds
//      `GBFRMeme.exe -c combat`                -> PlayerStats offset works
//
// 4. Commit. Note which game-build (steam manifest id / executable hash)
//    the file is keyed against in the commit message; the file itself is
//    not multi-build aware.
// =========================================================================
#pragma once

#include "common.hpp"

#include <cstddef>
#include <cstdint>

namespace gbfr::signatures {

// ---------------------------------------------------------------------------
// vftable RVAs
// ---------------------------------------------------------------------------
namespace vft {

// ----- engine root ---------------------------------------------------------
inline constexpr Address kMainLoop                   = 0x145153748ULL;
inline constexpr Address kAppMainLoop                = 0x145153708ULL;

// ----- camera --------------------------------------------------------------
inline constexpr Address kCameraBase                 = 0x14480b6d0ULL; // Fw::cCameraBase
inline constexpr Address kCameraApp                  = 0x1447fdb88ULL;
inline constexpr Address kCameraGame                 = 0x1447fda38ULL;
inline constexpr Address kCameraBattleCutscene       = 0x1447fdef8ULL;

// ----- managers ------------------------------------------------------------
inline constexpr Address kNpcAppearanceManager       = 0x1447fe390ULL;
inline constexpr Address kNetworkSystemRpcManager    = 0x1447fe5e0ULL;
inline constexpr Address kNetworkInitNetworkState    = 0x1447fe778ULL;
inline constexpr Address kNetworkInitMultiPlayState  = 0x1447fe9c8ULL;
inline constexpr Address kPhotoManager               = 0x145161d78ULL;

// ----- save-data modules ---------------------------------------------------
inline constexpr Address kSaveDataInitModule         = 0x145153780ULL;
inline constexpr Address kSaveDataReadModule         = 0x1451537c0ULL;
inline constexpr Address kSaveDataWriteModule        = 0x1451537a0ULL;
inline constexpr Address kSaveDataDeleteModule       = 0x1451537e0ULL;

// SaveDataWriteModule task lambdas (`std::_Func_impl_no_alloc<lambda, void>`).
inline constexpr Address kTaskRequestBuildCommonData         = 0x144820ff8ULL;
inline constexpr Address kTaskRequestBuildGraphicsSettingData = 0x1448210b8ULL;
inline constexpr Address kTaskEntryWriteSlotData             = 0x144821158ULL;
inline constexpr Address kTaskEntryWriteSlotInfo             = 0x1448211f8ULL;

// ----- quest -------------------------------------------------------------
inline constexpr Address kProgressManager            = 0x1447fc698ULL;
inline constexpr Address kMainQuestManager           = 0x145153cd8ULL;
inline constexpr Address kMultiQuestManager          = 0x145153d58ULL;
inline constexpr Address kFateEpisodeManager         = 0x145153d98ULL;
inline constexpr Address kBaseTownQuestManager       = 0x145153dd8ULL;
inline constexpr Address kTrialBattleManager         = 0x145153e18ULL;
inline constexpr Address kChallengeMissionManager    = 0x145153e58ULL;
inline constexpr Address kShortStoryQuestManager     = 0x145153e98ULL;

// ----- sys::data::*List (SaveListBase derivatives) ------------------------
inline constexpr Address kScenarioList               = 0x1451538c8ULL;
inline constexpr Address kFateEpList                 = 0x145153a40ULL;
inline constexpr Address kIslandList                 = 0x145153a68ULL;
inline constexpr Address kGachaList                  = 0x145153b10ULL;
inline constexpr Address kArchiveList                = 0x145153b38ULL;
inline constexpr Address kCollectiblesEmList         = 0x145153ba0ULL;
inline constexpr Address kCollectiblesBaList         = 0x145153c08ULL;
inline constexpr Address kCollectiblesChestList      = 0x145153c70ULL;
inline constexpr Address kAbilityList                = 0x1451547b8ULL;
inline constexpr Address kTradeList                  = 0x145154820ULL;
inline constexpr Address kMainStoryList              = 0x1451548c8ULL;
inline constexpr Address kBGMList                    = 0x145154930ULL;
inline constexpr Address kPictureBookCharaList       = 0x145154998ULL;
inline constexpr Address kPictureBookEnemyList       = 0x145154a00ULL;
inline constexpr Address kPictureBookPendulumList    = 0x145154a68ULL;
inline constexpr Address kTipsList                   = 0x145154ad0ULL;
inline constexpr Address kInfomationQuestList        = 0x1451579a0ULL;
inline constexpr Address kInfomationDialogList       = 0x145157a08ULL;
inline constexpr Address kCharaList                  = 0x145159ef8ULL;
inline constexpr Address kCharaPresetList            = 0x145159f60ULL;
inline constexpr Address kWeaponIdSaveList           = 0x145159f88ULL;
inline constexpr Address kGemIdSaveList              = 0x145159ff0ULL;
inline constexpr Address kItemList                   = 0x14515a058ULL;
inline constexpr Address kWordlistList               = 0x14515a180ULL;
inline constexpr Address kMenuUnlockSaveData         = 0x14515a1a8ULL;
inline constexpr Address kTutorialList               = 0x145161e38ULL;
inline constexpr Address kMenuTutorialList           = 0x145161ea0ULL;
inline constexpr Address kNpcVoiceList               = 0x145161f08ULL;
inline constexpr Address kCommandComboList           = 0x145192fc8ULL;

// ----- sys::data::*List (SaveTempListBase derivatives) --------------------
inline constexpr Address kGemList                    = 0x145153930ULL;
inline constexpr Address kItemPendulumList           = 0x145153998ULL;

// ----- typed save-field wrappers ------------------------------------------
// `SaveDataUnit<T,1>` is a 0x20-byte wrapper around a pointer to a single
// scalar save field. Layout (verified via Ghidra `IdentifyVftables` script):
//   +0x00 vftable
//   +0x08 refcount/flag (=1 when live)
//   +0x10 ptr to live storage (the actual scalar lives here in the save blob)
//   +0x18 cached copy of *(ptr) as a 4-byte value (+0x1C pad)
inline constexpr Address kSaveDataUnitInt1  = 0x144800AA0ULL; // SaveDataUnit<int,1>
inline constexpr Address kSaveDataUnitUint1 = 0x1447FF120ULL; // SaveDataUnit<unsigned int,1>

} // namespace vft

// ---------------------------------------------------------------------------
// Class-name strings (`s_*` labels in .rdata, used by cyan::Singleton<T>)
// ---------------------------------------------------------------------------
namespace name_string {

inline constexpr Address kCharacterManager           = 0x1451b1b2aULL;
inline constexpr Address kItemManager                = 0x1451b1c29ULL;
inline constexpr Address kUserDataManager            = 0x1451b1d7aULL;

} // namespace name_string

// ---------------------------------------------------------------------------
// Known function RVAs
// ---------------------------------------------------------------------------
namespace func {

inline constexpr Address kAppMainLoopBoot     = 0x140079040ULL; // vftable slot 1
inline constexpr Address kAppMainLoopTick     = 0x14007f460ULL; // vftable slot 2
inline constexpr Address kAppMainLoopShutdown = 0x140148360ULL; // vftable slot 3
inline constexpr Address kAppMainLoopCtor     = 0x140194430ULL; // installs vftable

} // namespace func

// ---------------------------------------------------------------------------
// Fixed counts / array sizes recovered from RTTI
// ---------------------------------------------------------------------------
namespace count {

inline constexpr std::size_t kPartySize = 4;

// Hard caps on the SaveListBase-derived list classes. The engine
// preallocates these slots and uses a sentinel key (signatures::sentinel::
// kEmptyKey) to indicate empty rows; the SDK walks all slots up to these
// caps when enumerating live entries.
inline constexpr std::uint32_t kItemListMax         = 300;
inline constexpr std::uint32_t kGemListMax          = 5100;
inline constexpr std::uint32_t kItemPendulumListMax = 5000;

} // namespace count

// ---------------------------------------------------------------------------
// Engine-wide sentinel values.
// ---------------------------------------------------------------------------
namespace sentinel {

// `string_hash32("")` under the custom xxh32 seed. The engine uses this
// value as the "empty slot" marker in every hashed save list (ItemList,
// GemList, ItemPendulumList, CharaList key, ...) and in the unequipped-
// character marker on sigils.
inline constexpr std::uint32_t kEmptyKey = 0x887AE0B0u;

} // namespace sentinel

// ---------------------------------------------------------------------------
// Byte patterns scanned for in `.text` to recover dynamic offsets.
// ---------------------------------------------------------------------------
namespace pattern {

// Locates the `LEA RCX, [RSI + disp32]` that materialises a pointer to the
// per-entity PlayerStats struct. `kPlayerDataOffsetDispPos` is the byte
// offset of the disp32 within the matched pattern.
inline constexpr const char* kPlayerDataOffset =
    "3D B0 E0 7A 88 0F ?? ?? ?? ?? ?? B8 B0 E0 7A 88 48 8D 8E ?? ?? ?? ??";
inline constexpr std::ptrdiff_t kPlayerDataOffsetDispPos = 19;

} // namespace pattern

// ---------------------------------------------------------------------------
// Static instance addresses ("singletons that live at a fixed RVA").
// These are NOT pointer slots — they are the object itself, allocated in the
// engine's `.data` section by a CRT static initialiser or in-place inside
// AppMainLoop's startup.
// ---------------------------------------------------------------------------
namespace instance {

// FUN_140194430 (AppMainLoop ctor) at 0x140194455 writes the vftable here.
inline constexpr Address kAppMainLoop            = 0x145e47650ULL;

// FUN_140211f60 (cCameraGame ctor) at 0x14021216a writes the vftable here.
inline constexpr Address kCameraGame             = 0x1468b4f90ULL;

// FUN_140194430 (AppMainLoop ctor) at 0x14019495c writes the vftable here.
// (Sub-object of AppMainLoop at offset 0x920.)
inline constexpr Address kNpcAppearanceManager   = 0x145e47f70ULL;

// Four static `cCameraApp`-derived instances (all share base `Fw::cCameraBase`
// and intermediate `cCameraApp`). The first three were introduced in
// FUN_140214000 around offsets 0x14021403B, 0x14021414B, 0x14021425B; the
// fourth is the `cCameraGame` itself reused above.
inline constexpr Address kCameraApp0             = 0x146093dd0ULL;
inline constexpr Address kCameraApp1             = 0x145e4a360ULL;
inline constexpr Address kCameraApp2             = 0x145e4a730ULL;

// Out-of-frustum / streaming manager (vftable 0x1448090C8). The Impl is at
// inst+0x18.
inline constexpr Address kOtManager              = 0x146741020ULL;

// Hit-flash effect manager.
inline constexpr Address kHitflashManager        = 0x1468b7500ULL;

// Photo-mode prohibit/impossible state machines (sub-objects of PhotoManager
// data). These are co-located in `.data` near the PhotoManager pointer slot.
inline constexpr Address kPhotoProhibitManager   = 0x145e54a98ULL;
inline constexpr Address kPhotoImpossibleManager = 0x145e54ae8ULL;

// NOTE: `Graphine::LogManager` is reported as a static at 0x146830220 by the
// vftable XREF scan, but the live qword there is zero — either lazy init or
// it's a `.bss` slot the manager is *moved into* later. Not listed here.
//
// NOTE: `BaOminousFormManager` was reported as a pointer slot at 0x14683BD88
// but its dereferenced value lands back inside the module (it's actually a
// `.data` sub-object), not a heap manager. Not listed here.

} // namespace instance

// ---------------------------------------------------------------------------
// Static pointer slots — these hold a pointer to a heap-allocated manager
// (the manager itself lives elsewhere; dereference to get the address).
// ---------------------------------------------------------------------------
namespace ptr_slot {

// FUN_14007f460 stores the live heap address of PhotoManager here.
inline constexpr Address kPhotoManager           = 0x145e54e20ULL;

// FUN_14007f460 stores aliases of NetworkSystemRpcManager.
inline constexpr Address kNetworkSystemRpcManager = 0x145fc7c70ULL;

// FUN_14007f460 stores the heap pointer to a stage::quest::ProgressManager-like instance.
inline constexpr Address kProgressManager        = 0x14683a800ULL;

// Stage object placement manager.
inline constexpr Address kStagePlacementManager  = 0x1467345a0ULL;

// Navigation-mesh manager (for AI pathing).
inline constexpr Address kNavimeshManager        = 0x1468cfbd8ULL;

// Renderer-side scene/composition managers.
inline constexpr Address kSceneObjectManager     = 0x1468469e0ULL;
inline constexpr Address kSubsurfaceShadingManager = 0x14612df00ULL;

// Hardware/platform user manager (Steam-side wrapper).
inline constexpr Address kHwUserManagerImpl      = 0x145fc9328ULL;

// Granite virtual-texture upload pipeline.
inline constexpr Address kGraniteUploadManager   = 0x145c84b20ULL;

// Wwise sound dispatcher.
inline constexpr Address kSoundDefaultSoftCallManager = 0x146855838ULL;

// Entity / object registry (discovered via cCameraGame slot-7 getTarget at
// FUN_1408991A0). Holds the live `cyan::ObjectRegistry`-style hashmap:
//   *(slot)            -> registry object
//   registry + 0x20    -> u64 key array
//   registry + 0x48    -> u64 value array
// `EntityHandle` keys map to live `Entity*` values through this.
inline constexpr Address kEntityRegistry         = 0x145e4b678ULL;

// SaveDataManager static pointer slot. Loaded by every SaveDataUnit
// installer in `FUN_14007f460` (`MOV RAX, qword ptr [0x145fcae68]`). The
// dereferenced object holds the construction context for save fields and a
// `[+0x8a0]` flag that gates registration. Not used yet for currency
// resolution (we fingerprint via vftable scan instead), but kept here as
// the canonical static anchor for the save data subsystem.
inline constexpr Address kSaveDataManager        = 0x145fcae68ULL;

} // namespace ptr_slot

// ---------------------------------------------------------------------------
// Verified field offsets land here once confirmed at runtime. Until then,
// wrappers use `GBFR_TODO_OFFSET(...)` directly so the gap is visible.
// ---------------------------------------------------------------------------
namespace offset {

// table::SaveListBase<TData, Key, MaxCount> layout. Verified by inspecting
// the in-tick constructor (FUN_14007f460) for `sys::data::CharaList` and the
// per-entry copy loop in FUN_140a68fa0: 40 slots of stride 0x3120 starting
// at +0x10. Each entry carries a `liveness` qword at +0x08 (sentinel
// `0xFFFFFFFFFFFFFFFF` == empty) and the entry's key (character id hash for
// CharaData) at +0x10.
namespace save_list_base {
    inline constexpr std::ptrdiff_t kVftable        = 0x00;
    inline constexpr std::ptrdiff_t kEntriesArray   = 0x10;
} // namespace save_list_base

// Field offsets inside `sys::data::CharaData` (one entry of CharaList).
// Entry stride is 0x3120 bytes total, confirmed by IMUL R14, R12, 0x3120.
namespace chara_data {
    inline constexpr std::ptrdiff_t kLivenessQword = 0x08;  // 0xFFFFFFFFFFFFFFFF when slot empty
    inline constexpr std::ptrdiff_t kKey           = 0x10;  // string_hash32 of character id
    // Remaining offsets (level, exp, costume, color) still TBD.
} // namespace chara_data

// Offsets of `sys::data::*List` inside the parent save-data aggregate
// (the world struct constructed by AppMainLoop slot 2 / FUN_14007f460).
//
// Verified for CharaList and CharaPresetList by reading the assembly window
// around 14008ac1f / 14008ac3c.
namespace save_aggregate {
    inline constexpr std::ptrdiff_t kCharaList       = 0xd70;
    inline constexpr std::ptrdiff_t kCharaPresetList = 0x7bac0;
    // Remaining list offsets (CharaList ... is contiguous, see vftable
    // RVA cluster) still TBD.
} // namespace save_aggregate

// `SaveDataUnit<T,1>` wrapper layout (0x20 bytes).
namespace save_data_unit {
    inline constexpr std::ptrdiff_t kVftable     = 0x00;
    inline constexpr std::ptrdiff_t kFlag        = 0x08;
    inline constexpr std::ptrdiff_t kSrcPtr      = 0x10;
    inline constexpr std::ptrdiff_t kCachedValue = 0x18;
    inline constexpr std::size_t    kRecordSize  = 0x20;
} // namespace save_data_unit

// Offsets within the "user save block" — a heap struct that holds the
// active save slot's wallet (rupies, mastery points), player display name,
// and various achievement bitfields. Located by SaveDataUnit<int,1>
// fingerprint: rupie's `src_ptr` is the block base; mastery's `src_ptr`
// equals base + 0x68. Verified live with rupies=863326879, mastery=99999.
namespace user_save_block {
    inline constexpr std::ptrdiff_t kRupiesU32        = 0x00;
    inline constexpr std::ptrdiff_t kSlotActiveFlag   = 0x08; // u32, == 1 when active
    inline constexpr std::ptrdiff_t kPlayerNameUtf16  = 0x0C; // 64-byte UTF-16 buffer
    inline constexpr std::size_t    kPlayerNameSize   = 0x40;
    inline constexpr std::ptrdiff_t kMasteryPointsU32 = 0x68;
} // namespace user_save_block

// Engine list layouts. Each list is a SaveListBase / SaveTempListBase
// derivative; the `kEntriesOffset` value is where the entry array begins
// inside the list object.
namespace item_list {
    inline constexpr std::ptrdiff_t kEntriesOffset = 0x10;
    inline constexpr std::ptrdiff_t kEntryStride   = 0x20;
} // namespace item_list

namespace gem_list {
    // GemList is a SaveTempListBase variant: entries begin at +0x08 (not
    // the usual +0x10 of SaveListBase derivatives).
    inline constexpr std::ptrdiff_t kEntriesOffset = 0x08;
    inline constexpr std::ptrdiff_t kEntryStride   = 0x24;
} // namespace gem_list

namespace item_pendulum_list {
    inline constexpr std::ptrdiff_t kEntriesOffset = 0x10;
    inline constexpr std::ptrdiff_t kEntryStride   = 0x30;
} // namespace item_pendulum_list

} // namespace offset

} // namespace gbfr::signatures
