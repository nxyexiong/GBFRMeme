# GBFRMeme

A C++20 toolkit for inspecting **Granblue Fantasy: Relink**
(`granblue_fantasy_relink.exe`) at runtime. It exposes a typed SDK of the
game's internal structures and a small DLL that can be loaded into (or
attached from outside) the running game process.

> **Status:** early scaffolding. The class hierarchy, manager registry, save
> data list cluster and main-loop control flow are mapped out, but most byte
> offsets inside individual game structs are still marked
> `GBFR_TODO_OFFSET(...)` until they're verified against decompilation or
> live memory.

## Repository layout

```text
GBFRMeme/
├── CMakeLists.txt              top-level build (adds src/sdk then src/core)
└── src/
    ├── sdk/                    gbfr::sdk — header-only-ish, host-agnostic
    │   ├── CMakeLists.txt
    │   ├── include/gbfr/       public SDK headers (see below)
    │   └── src/game.cpp        Game facade implementation
    └── core/                   gbfr::core — DLL with attach + scanning
        ├── CMakeLists.txt
        ├── include/gbfr/core/  process_attach / signatures / session / c_api
        └── src/                memory backends, signature scanner, DLL entry
```

The two C++ targets:

| CMake target | Kind          | Depends on | Purpose                                              |
| ------------ | ------------- | ---------- | ---------------------------------------------------- |
| `gbfr::sdk`  | `STATIC` lib  | —          | Typed wrappers + RTTI-recovered constants. No I/O.   |
| `gbfr::core` | `SHARED` DLL  | `gbfr::sdk`, `psapi` | Memory backends, attach, signatures, stable C ABI. |

## Building

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Outputs land in `build/src/core/Release/gbfr_core.dll` and the corresponding
`.lib`. The SDK is consumed transitively; you generally only link
`gbfr::core` from a host.

## SDK — `src/sdk/include/gbfr/`

The SDK mirrors the game's class hierarchy. Every wrapper derives from
[`gbfr::GameObject`](src/sdk/include/gbfr/object.hpp) and reads fields through
a pluggable [`gbfr::IMemory`](src/sdk/include/gbfr/memory.hpp) backend, so the
same code works **in-process** (direct pointers) or **out-of-process**
(`ReadProcessMemory`).

| Header                                                | Wraps                                                                |
| ----------------------------------------------------- | -------------------------------------------------------------------- |
| [`common.hpp`](src/sdk/include/gbfr/common.hpp)       | `Address`, `StringHash32` (`"foo"_h32`), `EntityHandle`, `ObjId`, RVA helpers, `GBFR_TODO_OFFSET` macro |
| [`memory.hpp`](src/sdk/include/gbfr/memory.hpp)       | Abstract `IMemory` + typed `Memory::read<T>/write<T>` helpers        |
| [`object.hpp`](src/sdk/include/gbfr/object.hpp)       | `GameObject` base class                                              |
| [`entity.hpp`](src/sdk/include/gbfr/entity.hpp)       | `Entity` (heap object addressed by `EntityHandle`; no vftable)       |
| [`main_loop.hpp`](src/sdk/include/gbfr/main_loop.hpp) | `MainLoop` / `AppMainLoop` (root of the frame loop)                  |
| [`cameras/camera.hpp`](src/sdk/include/gbfr/cameras/camera.hpp) | `Fw::cCameraBase` → `cCameraApp` → `cCameraGame` chain   |
| [`managers/*.hpp`](src/sdk/include/gbfr/managers)     | `CharacterManager`, `ItemManager`, `UserDataManager`, `PlayerDataManager`, `SaveDataManager`, `NetworkSystemRpcManager`, `PhotoManager`, `NpcAppearanceManager`, `quest::QuestSystem`, `quest::ProgressManager` |
| [`save/save_list_base.hpp`](src/sdk/include/gbfr/save/save_list_base.hpp) | `table::SaveListBase<T,K,N>` / `table::SaveTempListBase<T,K,N,C>` |
| [`save/save_data_unit.hpp`](src/sdk/include/gbfr/save/save_data_unit.hpp) | Per-leaf `SaveDataUnit<T, N>`                            |
| [`save/data_rows.hpp`](src/sdk/include/gbfr/save/data_rows.hpp) | All `sys::data::*Data` row types (`CharaData`, `GemData`, …) |
| [`save/sys_data_lists.hpp`](src/sdk/include/gbfr/save/sys_data_lists.hpp) | The 30+ concrete `sys::data::*List` typedefs              |
| [`save/slot_data.hpp`](src/sdk/include/gbfr/save/slot_data.hpp) | `Slot` aggregate — one instance per save slot               |
| [`game.hpp`](src/sdk/include/gbfr/game.hpp)           | `Game` — top-level facade owning an `IMemory` and every manager address |

All vftable RVAs are encoded against the design-time image base
`0x140000000`. At runtime they are translated to live addresses via
`rva_to_absolute(module_base, rva)` in `common.hpp`.

## Core DLL — `src/core/include/gbfr/core/`

`gbfr::core` is the host glue. It does I/O, process attach and pattern
scanning, and exposes a **stable C ABI** so non-C++ hosts can drive it.

| Header                                                            | Provides                                                                 |
| ----------------------------------------------------------------- | ------------------------------------------------------------------------ |
| [`process_attach.hpp`](src/core/include/gbfr/core/process_attach.hpp) | `find_loaded_module(name)` → `ModuleInfo{base, size}`             |
| [`memory/internal_memory.hpp`](src/core/include/gbfr/core/memory/internal_memory.hpp) | `IMemory` backed by direct in-process pointers       |
| [`memory/external_memory.hpp`](src/core/include/gbfr/core/memory/external_memory.hpp) | `IMemory` backed by `ReadProcessMemory` / `WriteProcessMemory` |
| [`signatures.hpp`](src/core/include/gbfr/core/signatures.hpp)     | IDA-style `compile_pattern` + `find_pattern` + `find_lea_refs_to` heuristic for locating `cyan::Singleton<T>` installers |
| [`session.hpp`](src/core/include/gbfr/core/session.hpp)           | `Session::attach_in_process()` / `attach_external_by_name(...)`, owns the `IMemory` and the SDK `Game` |
| [`c_api.h`](src/core/include/gbfr/core/c_api.h)                   | The stable C entry points: `gbfr_init`, `gbfr_shutdown`, per-manager sections (placeholders for now) |

The DLL is `gbfr_core.dll`. `gbfr_api_version()` is reserved for ABI
bumping. All ABI functions are `__cdecl` and use the `GbfrStatus` enum.

---

# Game structure (Granblue Fantasy: Relink)

The constants encoded by the SDK (vftable RVAs, `MaxCount`s, template
parameters, manager class-name strings) were recovered from a headless
analysis of `granblue_fantasy_relink.exe` at image base `0x140000000`.

## Runtime: one polymorphic main loop

The whole game is a single `AppMainLoop : MainLoop` instance installed by the
CRT path through `FUN_140194430`. Its vftable lives at RVA `0x145153708`:

| Slot | Function    | Size   | Role                                                              |
| ---- | ----------- | ------ | ----------------------------------------------------------------- |
| 0    | `0x140198260` |  38 B | scalar deleting destructor                                        |
| 1    | `0x140079040` |  13 KB | boot/init — places `SaveDataReadModule`                          |
| 2    | `0x14007f460` | **328 KB** | **per-frame tick** — also placement-new's the `sys::data::*` list cluster on first run |
| 3    | `0x140148360` | 2.7 KB | shutdown/drain                                                    |
| 4–5  | `0x14007d530` |   1 B | no-op pure-virtual stubs                                          |

One frame = one call to slot 2. Everything else hangs off subsystems:

- **Renderer** — `fg::FrameGraph` → D3D11. Virtual texturing via
  `Graphine::Granite` (`UploadManager`, `ResidencyManager`).
- **Audio** — Wwise (`AK::*`, `CAk*`).
- **AI** — `BT::*` behavior trees (~4 081 classes) and per-enemy managers
  (`Em7700Manager`, …).
- **Stage / quest** — `stage::quest::QuestSystem`, `ProgressManager`, plus
  per-quest-type managers (Main, Multi, FateEpisode, BaseTown, TrialBattle,
  ChallengeMission, ShortStory). Stage placement via
  `stage::placement::{Manager,Generator}`.
- **NPCs** — `cNpcAppearanceManager`, `cAppearsNpcHandle`, `cAppearanceNpc`.
- **Camera** — `Fw::cCameraBase` ← `cCameraApp` ← `cCameraGame`, plus mode
  classes (`CameraBattleCutscene`, `CameraResult`, `CameraShop`,
  `CameraPhotoMode`, …).
- **Streaming / misc** — `cOtManager`, `NavimeshManager`, `hitflash::Manager`.

## Managers via `cyan::Singleton<T>`

The "manager" classes have **no vftable of their own** — they are accessed
through `cyan::Singleton<T>` keyed by `cyan::string_hash32(class_name)`.
Their class-name strings live as `s_*` labels in `.rdata` near `0x1451b1xxx`:

```
s_CharacterManager   @ 0x1451b1b2a
s_ItemManager        @ 0x1451b1c29
s_UserDataManager    @ 0x1451b1d7a
... (and so on, one per manager)
```

`gbfr::core::Session::resolve_singletons_via_name_strings()` reuses this:
walk `lea` references to those strings to find each manager's installer
function, and from there the static slot holding `this`.

## Player data, three layers in RAM

1. **Singleton managers** in `.data`: `CharacterManager`, `ItemManager`,
   `UserDataManager`, `PlayerDataManager`, `SaveDataManager`, `PhotoManager`,
   `cNpcAppearanceManager`, `NetworkSystemRpcManager`.
2. **Save-data aggregate** built by `FUN_14007f460`: a contiguous cluster of
   `sys::data::*List` objects (vftables in `0x1451538c8 .. 0x145193008`). This
   is the in-memory mirror of the slot save file.
3. **Per-entity runtime state** on the heap as `Entity` objects, reached via
   `EntityHandle` and indexed by `unordered_map<uint, cyan::raw_ptr<Entity>>`.
   `Entity` has no vftable; kinds are tagged with `eObjId`.

## Save file: four serialized regions

`SaveDataManager` drives four polymorphic modules
(`SaveDataInitModule`, `SaveDataReadModule`, `SaveDataWriteModule`,
`SaveDataDeleteModule`) keyed by `string_hash32` `CallData` records:

| Region              | Built by                                      | Contents                                                  |
| ------------------- | --------------------------------------------- | --------------------------------------------------------- |
| **Slot data**       | `taskEntryWriteSlotData_`                     | The full `sys::data::*` list cluster (per-character progression, inventory, …) |
| **Slot info**       | `taskEntryWriteSlotInfo_`                     | Slot metadata: playtime, last save time, chapter, leader  |
| **Common data**     | `taskRequestBuildCommonData_`                 | Cross-slot / persistent unlocks                           |
| **Graphics setting**| `taskRequestBuildGraphicsSettingData_`        | Video options                                             |

Slot data uses `table::SaveListBase<TData, KeyType, MaxCount>` (and
`SaveTempListBase<…, ChunkCount>` for stackable inventories). Each leaf field
is wrapped in a `SaveDataUnit<T, N>` (>70 distinct instantiations).

### Slot-data list cluster (highlights)

| List                                 | Max  | Keyed by      | Holds                                          |
| ------------------------------------ | ---- | ------------- | ---------------------------------------------- |
| `sys::data::CharaList`               | 40   | `hash32`      | Per-character: level, exp, costume, color, …   |
| `sys::data::CharaPresetList`         | 40   | `hash32`      | Character presets                              |
| `sys::data::WeaponIdSaveList`        | 512  | `hash32`      | Owned weapons (by asset id hash)               |
| `sys::data::GemIdSaveList`           | 900  | `hash32`      | Sigil presets / loadouts                       |
| `sys::data::GemList`                 | **5100** | `uint`    | Sigil inventory — rolled instances             |
| `sys::data::ItemList`                | 300  | `hash32`      | Consumables / materials                        |
| `sys::data::ItemPendulumList`        | 5000 | `uint`        | Wrightstones                                   |
| `sys::data::AbilityList`             | 640  | `hash32`      | Skill / ability unlocks                        |
| `sys::data::ScenarioList`            | 64   | `hash32`      | Active scenarios                               |
| `sys::data::FateEpList`              | 800  | `hash32`      | Fate Episodes                                  |
| `sys::data::MainStoryList`           | 260  | `hash32`      | Main story flags                               |
| `sys::data::TradeList`               | 1024 | `hash32`      | Shop / trade progress                          |
| `sys::data::IslandList`              | 32   | `hash32`      | Discovered locations                           |
| `sys::data::CollectiblesEmList`      | 256  | `uint`        | Enemy collectibles                             |
| `sys::data::CollectiblesBaList`      | 256  | `uint`        | Boss collectibles                              |
| `sys::data::CollectiblesChestList`   | 256  | `hash32`      | Chest tracking                                 |
| `sys::data::BGMList`                 | 100  | `hash32`      | Unlocked BGM                                   |
| `sys::data::PictureBookCharaList`    | 50   | `hash32`      | Picture book — characters                      |
| `sys::data::PictureBookEnemyList`    | 150  | `hash32`      | Picture book — enemies                         |
| `sys::data::TipsList`                | 500  | `hash32`      | Tips read                                      |
| `sys::data::TutorialList`            | 300  | `hash32`      | Tutorials completed                            |
| `sys::data::MenuUnlockSaveData`      | 128  | `hash32`      | Menu unlocks                                   |
| `sys::data::CommandComboList`        | 800  | `hash32`      | Custom combo presets                           |

(Full table — including all 30+ lists — lives in the canonical doc.)

### Inventory (split four ways)

- **Consumables / materials** → `ItemList` (300)
- **Sigils** → `GemList` (5100) instances + `GemIdSaveList` (900) loadouts
- **Wrightstones** → `ItemPendulumList` (5000)
- **Weapons** → `WeaponIdSaveList` (512)

Static metadata is in `table::*Data` rows held as
`cyan::raw_ptr<table::*Data const>`.

### Character stats

- **Static** — `table::Chara*Data` rows in `.rdata` (`CharaData`,
  `CharaStatusData`, `CharaExpData`, `CharaGemData`, `CharaPowerAdjustData`,
  `CharaPowerAttenuateData` for Proud-mode scaling, `CharaLevelSyncData` for
  multiplayer, `CharaStatusFateData` for Fate-Episode bonuses, …).
- **Per-save** — `sys::data::CharaList` (40 slots).
- **Runtime cache** — `CharacterManager` owns
  `unordered_map<string_hash32, unique_ptr<PlayerEquipDataAccumulater>>` plus
  a 4-slot `array<pair<sys::data::MultiCharaData, PlayerEquipDataAccumulater>, 4>`
  indexed by `MultiCharaDataCacheType` (the four party slots).

### Combat-frame parameters

Per-aspect polymorphic objects on each player (vftables in
`0x144805cc8 .. 0x14480e638`): `PlayerMoveParameter`,
`PlayerBuffParameter`, `PlayerAilmentParameter`, `PlayerContributionParameter`,
`PlayerAbilityUIParameter`, `PlayerAutoHomingParameter`,
`PlayerLockOnParameter`, `PlayerAnimOverrideParameter`,
`PlayerHitEffectOverrideParameter`, `PlayerLinkAttackVoiceParameter`,
`PlayerDamageLimitParameter`.

## TL;DR

- The whole game is one `AppMainLoop` whose vftable slot 2 is the per-frame
  tick and also constructs the save-data list cluster at startup.
- The slot save is a struct full of `sys::data::*List` objects backed by
  `table::SaveListBase<T,K,N>` / `table::SaveTempListBase<...>`, with leaf
  fields serialized as `SaveDataUnit<T,N>` and keyed by
  `cyan::string_hash32`.
- Managers (`CharacterManager`, `ItemManager`, …) carry no vftable; they live
  behind the `cyan::Singleton<T>` registry.
- The SDK encodes the recovered RTTI constants (vftable RVAs, MaxCounts,
  template parameters); byte offsets that still need confirmation are marked
  `GBFR_TODO_OFFSET(...)`.
