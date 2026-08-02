// SPDX-License-Identifier: MIT
// gbfr/core/c_api.h — stable C ABI for `gbfr_core.dll`.
//
// Layout: lifecycle first, then one section per live game-data surface.
// Character support remains partial; inventory, sigil, wrightstone, summon,
// and currency sections expose read/write operations.
//
// Naming convention: `gbfr_<manager_snake_case>_<verb>`.
#ifndef GBFR_CORE_C_API_H
#define GBFR_CORE_C_API_H

#include <stddef.h>
#include <stdint.h>

#if defined(GBFR_CORE_BUILDING_DLL)
    #define GBFR_CORE_API __declspec(dllexport)
#else
    #define GBFR_CORE_API __declspec(dllimport)
#endif
#define GBFR_CORE_CALL __cdecl

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// 0. Common
// ---------------------------------------------------------------------------

typedef enum GbfrStatus {
    GBFR_OK                 = 0,
    GBFR_ERR_INVALID_ARG    = 1,
    GBFR_ERR_NOT_FOUND      = 2,    // game not running / module missing
    GBFR_ERR_ALREADY_INIT   = 3,
    GBFR_ERR_NOT_INIT       = 4,
    GBFR_ERR_NOT_AVAILABLE  = 5,    // queried data not yet wired up
    GBFR_ERR_PLATFORM       = 6,    // OS-level failure (e.g. RPM)
    GBFR_ERR_OUT_OF_RANGE   = 7,
} GbfrStatus;

typedef enum GbfrAttachMode {
    GBFR_ATTACH_INTERNAL = 0,   // running inside the game process
    GBFR_ATTACH_EXTERNAL = 1,   // attaches to a foreign game process
} GbfrAttachMode;

// Bit flags for `gbfr_init`.
#define GBFR_INIT_NONE  0u
#define GBFR_INIT_NO_UI 1u   // Skip starting the GUI window (CLI / headless).

// ---------------------------------------------------------------------------
// 1. Lifecycle
// ---------------------------------------------------------------------------

GBFR_CORE_API uint32_t   GBFR_CORE_CALL gbfr_api_version(void);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_init(GbfrAttachMode mode,
                                                  const wchar_t*  executable_name,
                                                  uint32_t        flags);
GBFR_CORE_API void       GBFR_CORE_CALL gbfr_shutdown(void);
GBFR_CORE_API int        GBFR_CORE_CALL gbfr_is_initialized(void);

// Block the calling thread until the UI host exits. For EXTERNAL/CLI
// attach modes this returns once the user closes the host window. For
// INTERNAL mode (and when no session is initialised) it returns immediately.
GBFR_CORE_API void       GBFR_CORE_CALL gbfr_wait_for_exit(void);

// ---------------------------------------------------------------------------
// 2. CharacterManager  (save-data view)
//    Per-character non-combat state: level, exp, costume, color.
//    Characters are enumerated by opaque index 0..count-1; ordering is
//    stable for the lifetime of a session.
// ---------------------------------------------------------------------------

typedef struct GbfrCharacterInfo {
    char     name_id[64];      // internal asset id, e.g. "io"
    char     display_name[64]; // localised in-game name (empty if not recovered)
    uint32_t level;
    uint64_t exp;
    char     costume_id[64];
    uint32_t color_index;
} GbfrCharacterInfo;

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_character_get_count(uint32_t* out_count);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_character_get_at(uint32_t index,
                                                              GbfrCharacterInfo* out_info);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_character_get_by_name(const char* name_id,
                                                                   GbfrCharacterInfo* out_info);

// ---------------------------------------------------------------------------
// 3. Live combat
//    State of the character the local player is currently controlling.
// ---------------------------------------------------------------------------

typedef struct GbfrCombatInfo {
    char     player_name[64];             // local player display name
    char     character_name_id[64];       // controlled character's asset id
    char     character_display_name[64];  // localised name
    uint32_t level;
    float    hp;
    float    hp_max;
    float    critical_rate;               // 0..100 (percent)
} GbfrCombatInfo;

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_combat_get_current_info(GbfrCombatInfo* out_info);

// Populate `out_info` by reading from an explicit Entity pointer. Used
// when entity discovery has been handled out-of-band (e.g. the user
// supplies an address found via `debug` scans, or an injected hook fires).
// Reads PlayerStats at `entity + player_data_offset` (verified per build).
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_combat_read_from_entity(
    uint64_t entity_addr, GbfrCombatInfo* out_info);

// ---------------------------------------------------------------------------
// 5. Inventory
//    `sys::data::ItemList` walking. The list is heap-allocated; the first
//    call performs a process-wide vftable scan, the result is cached for
//    the lifetime of the session.
// ---------------------------------------------------------------------------

typedef struct GbfrItemInfo {
    uint32_t item_hash;       // string_hash32 of the item's asset id
    uint32_t count;           // quantity owned (0..999 typical)
    uint32_t acquired_seq;    // engine-internal acquisition order / timestamp
    uint32_t type_tag;        // engine-internal type discriminator (0x04, 0x0C)
    char     asset_id[64];    // human-readable asset id (e.g. "ITEM_01_0000")
    char     display_name[64];// localised in-game name (e.g. "Cobblestone")
} GbfrItemInfo;

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_item_get_count(uint32_t* out_count);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_item_get_at(uint32_t index,
                                                         GbfrItemInfo* out_info);
// Write a new `count` value for the live ItemList entry currently exposed at
// `index` (same indexing as `gbfr_item_get_at`). Caller is responsible for
// staying within the engine's clamp range (1..999 is typical).
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_item_set_count(uint32_t index,
                                                            uint32_t count);

// Look up an item by its hash. Useful for cross-referencing other APIs
// (gem traits, weapon materials, etc.) that surface item hashes.
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_item_lookup(
    uint32_t hash, char* out_asset_id, char* out_name);

// ---------------------------------------------------------------------------
// 6. Sigils (sys::data::GemList walking)
//
// Each rolled sigil instance lives here. The list is heap-allocated; first
// call locates it via a vftable scan, then cached.
// ---------------------------------------------------------------------------

typedef struct GbfrSigilInfo {
    uint32_t first_trait_id;
    uint32_t first_trait_level;
    uint32_t second_trait_id;
    uint32_t second_trait_level;
    uint32_t sigil_id;
    uint32_t equipped_character;  // hash32 of Pl#### or 0x887AE0B0 if unequipped
    uint32_t sigil_level;
    uint32_t acquisition_count;
    uint32_t notification_enum;
    // Human-readable trait/sigil names (looked up via the item table).
    char     first_trait_name[64];
    char     second_trait_name[64];
    char     sigil_name[64];
    char     equipped_character_id[64]; // "Pl0500" or "" if unequipped
} GbfrSigilInfo;

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_sigil_get_count(uint32_t* out_count);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_sigil_get_at(uint32_t index,
                                                          GbfrSigilInfo* out_info);
// Update fields on the live GemList entry currently exposed at `index`.
// `sigil_level` writes the base sigil level. `trait1_name` and
// `trait2_name` accept a case-insensitive display name or exact `SKILL_*`
// asset ID; pass NULL or empty to leave that trait unchanged. Ambiguous
// display names return GBFR_ERR_NOT_FOUND unless they match the current
// trait, preserving unchanged editor round-trips.
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_sigil_set_fields(
    uint32_t    index,
    uint32_t    sigil_level,
    const char* trait1_name,
    uint32_t    trait1_level,
    const char* trait2_name,
    uint32_t    trait2_level);

// ---------------------------------------------------------------------------
// 7. Wrightstones (sys::data::ItemPendulumList walking)
//
// Each rolled Wrightstone instance lives here with up to 3 traits.
// ---------------------------------------------------------------------------

typedef struct GbfrWrightstoneInfo {
    uint32_t trait1_id;
    uint32_t trait1_level;
    uint32_t trait2_id;
    uint32_t trait2_level;
    uint32_t trait3_id;
    uint32_t trait3_level;
    uint32_t template_hash;     // ITEM_25/26/27/28_xxxx hash
    uint32_t acquired_seq;
    uint32_t notification_enum;
    char     trait1_name[64];
    char     trait2_name[64];
    char     trait3_name[64];
    char     template_asset_id[64];
    char     template_name[64];
} GbfrWrightstoneInfo;

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_wrightstone_get_count(uint32_t* out_count);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_wrightstone_get_at(uint32_t index,
                                                                GbfrWrightstoneInfo* out_info);
// Update fields on the live ItemPendulumList entry currently exposed at
// `index`. Each `traitN_name` accepts a case-insensitive display name or
// exact `SKILL_*` asset ID; pass NULL or empty to leave that trait's hash
// unchanged (level is still written). Ambiguous display names return
// GBFR_ERR_NOT_FOUND unless they match the current trait.
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_wrightstone_set_fields(
    uint32_t    index,
    const char* trait1_name, uint32_t trait1_level,
    const char* trait2_name, uint32_t trait2_level,
    const char* trait3_name, uint32_t trait3_level);

// ---------------------------------------------------------------------------
// 8. Summons (sys::data::SummonStoneList walking)
//
// Each owned summon has one trait and one stat/equip bonus.
// ---------------------------------------------------------------------------

typedef struct GbfrSummonInfo {
    uint32_t storage_index;   // stable physical slot within this session
    uint32_t summon_id;
    uint32_t unknown_04;
    uint32_t trait_id;
    uint32_t stat_type_id;
    uint32_t trait_level;
    uint32_t stat_level;       // internal 0-based index into the stat value table
    uint32_t stat_value;
    uint32_t stat_is_percent;
    uint32_t unknown_18;
    char     summon_asset_id[64];
    char     summon_name[64];
    char     trait_asset_id[64];
    char     trait_name[64];
    char     stat_type_asset_id[64];
    char     stat_type_name[64];
} GbfrSummonInfo;

#define GBFR_SUMMON_STAT_VALUE_COUNT 10u

typedef struct GbfrSummonStatTypeInfo {
    uint32_t stat_type_id;
    uint32_t values[GBFR_SUMMON_STAT_VALUE_COUNT];
    uint32_t is_percent;
    char     asset_id[64];
    char     name[64];
} GbfrSummonStatTypeInfo;

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_summon_get_count(uint32_t* out_count);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_summon_get_at(uint32_t index,
                                                           GbfrSummonInfo* out_info);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_summon_get_by_storage_index(
    uint32_t storage_index, GbfrSummonInfo* out_info);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_summon_stat_type_get_count(
    uint32_t* out_count);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_summon_stat_type_get_at(
    uint32_t index, GbfrSummonStatTypeInfo* out_info);

// Update an owned summon by stable physical slot. The expected identity
// fields prevent a stale UI row from modifying a replacement occupant.
// `trait_name` accepts a case-insensitive display name or exact SKILL_* asset
// ID; pass NULL or empty to preserve its ID.
// `stat_value` is the displayed value (e.g. 2000 ATK or 30 percent), not the
// internal 0-based stat level. Returns OUT_OF_RANGE when that value is not
// valid for `stat_type_id`.
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_summon_set_fields(
    uint32_t    storage_index,
    uint32_t    expected_summon_id,
    uint32_t    expected_unknown_04,
    const char* trait_name,
    uint32_t    trait_level,
    uint32_t    stat_type_id,
    uint32_t    stat_value);

// ---------------------------------------------------------------------------
// 3.5 Currencies (rupies, mastery points).
// ---------------------------------------------------------------------------

typedef struct GbfrCurrencyInfo {
    uint64_t wallet_address;  // live address of the user save block (debug)
    uint32_t rupies;          // u32 in-game value
    uint32_t mastery_points;  // u32 in-game value
} GbfrCurrencyInfo;

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_currency_get_info(GbfrCurrencyInfo* out_info);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_currency_set_rupies(uint32_t value);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_currency_set_mastery_points(uint32_t value);

// ---------------------------------------------------------------------------
// 4. Debug primitives (not part of the stable game-level API).
//    Backdoor for the CLI / reverse-engineering session. Subject to change.
// ---------------------------------------------------------------------------

typedef struct GbfrModuleInfo {
    uint64_t base;
    uint64_t size;
} GbfrModuleInfo;

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_module_info(GbfrModuleInfo* out);

// Convert a Ghidra-style RVA (image base 0x140000000) into a live address.
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_rva_to_abs(uint64_t rva, uint64_t* out_abs);

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_u8 (uint64_t addr, uint8_t*  out);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_u32(uint64_t addr, uint32_t* out);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_u64(uint64_t addr, uint64_t* out);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_f32(uint64_t addr, float*    out);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_bytes(uint64_t addr, void* out, uint32_t n);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_cstr(uint64_t addr, char* out, uint32_t cap);

// First occurrence; 0 on not-found is encoded as GBFR_ERR_NOT_FOUND.
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_qword(uint64_t value, uint64_t* out_addr);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_pattern(const char* ida_pattern, uint64_t* out_addr);

// Up to `cap` matches. Writes `*out_count` (also when no caller buffer is
// supplied so the caller can size a buffer; pass `out_addrs = NULL, cap = 0`).
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_qword_all(
    uint64_t value, uint64_t* out_addrs, uint32_t cap, uint32_t* out_count);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_qword_in_process(
    uint64_t value, uint64_t* out_addrs, uint32_t cap, uint32_t* out_count);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_dword_in_process(
    uint32_t value, uint64_t* out_addrs, uint32_t cap, uint32_t* out_count);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_pattern_all(
    const char* ida_pattern, uint64_t* out_addrs, uint32_t cap, uint32_t* out_count);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_find_lea_refs_to(
    uint64_t target, uint64_t* out_addrs, uint32_t cap, uint32_t* out_count);

// Resolve the live save-data aggregate base (one process scan, cached).
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_save_aggregate(uint64_t* out_addr);

// Offset of `PlayerStats` relative to a player Entity. Found by signature
// scan, cached. 0 means "not found in this build".
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_player_data_offset(uint32_t* out_off);

// Live address of the static AppMainLoop instance (zero scans — it lives at
// a fixed RVA verified against the constructor). Returns OK with the
// resolved address even if the object's vftable isn't installed yet.
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_app_main_loop(uint64_t* out_addr);

// Compute the engine's 32-bit string hash (FNV-1a 32) of a UTF-8 string.
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_hash32(const char* s, uint32_t* out_hash);

// Resolve a character_type hash to its canonical asset id (e.g. "Pl0500") and
// human-readable name ("Eugen"). Returns GBFR_ERR_NOT_FOUND if the hash isn't
// in the baked table. `out_asset_id` / `out_name` must point to caller buffers
// of at least 64 chars; either may be NULL.
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_character_type_lookup(
    uint32_t hash, char* out_asset_id, char* out_name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GBFR_CORE_C_API_H
