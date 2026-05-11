// SPDX-License-Identifier: MIT
// gbfr/core/c_api.h — stable C ABI for `gbfr_core.dll`.
//
// Layout: lifecycle first, then one section per "manager" mirroring the
// runtime layout of the game (see
// `reverse/20260510/docs/scene-and-save-structure.md`). No section has any
// real query yet — placeholders only. The shape will be filled in as the
// SDK's field offsets are recovered.
//
// Naming convention: `gbfr_<manager_snake_case>_<verb>`.
#ifndef GBFR_CORE_C_API_H
#define GBFR_CORE_C_API_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
    #if defined(GBFR_CORE_BUILDING_DLL)
        #define GBFR_CORE_API __declspec(dllexport)
    #else
        #define GBFR_CORE_API __declspec(dllimport)
    #endif
    #define GBFR_CORE_CALL __cdecl
#else
    #define GBFR_CORE_API __attribute__((visibility("default")))
    #define GBFR_CORE_CALL
#endif

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

// ---------------------------------------------------------------------------
// 1. Lifecycle
// ---------------------------------------------------------------------------

GBFR_CORE_API uint32_t   GBFR_CORE_CALL gbfr_api_version(void);
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_init(GbfrAttachMode mode, const wchar_t* executable_name);
GBFR_CORE_API void       GBFR_CORE_CALL gbfr_shutdown(void);
GBFR_CORE_API int        GBFR_CORE_CALL gbfr_is_initialized(void);

// ---------------------------------------------------------------------------
// 2. CharacterManager
//    Per-character runtime state (level, exp, equipment) and the 4 party
//    slots (`PlayerEquipDataAccumulater[4]`).
// ---------------------------------------------------------------------------

// (no operations yet)

// ---------------------------------------------------------------------------
// 3. ItemManager
//    Consumables / materials inventory and reward generation
//    (`createRewardBySet` etc.).
// ---------------------------------------------------------------------------

// (no operations yet)

// ---------------------------------------------------------------------------
// 4. PlayerDataManager
//    Multiplayer player cache (`NetworkReadCacheData`).
// ---------------------------------------------------------------------------

// (no operations yet)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GBFR_CORE_C_API_H
