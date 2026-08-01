// SPDX-License-Identifier: MIT
// gbfr/character_types.hpp — playable-character hash table.
//
// Maps the engine's `string_hash32` of each character's actor id (e.g.
// `"Pl0000"` -> `0x26A4848A`) to its asset prefix and human-readable name.
//
// Source: cross-referenced from
//   - villith/relink-logs: src-tauri/src/parser/constants.rs
//   - nenkai.github.io/relink-modding: character pages
//
// The hash function is `gbfr::StringHash32` (custom xxHash32) — verified
// here by recomputing each row at compile time; if any drift, the assertion
// catches it at build.
#pragma once

#include "common.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace gbfr {

struct CharacterTypeEntry {
    std::uint32_t    hash;       // string_hash32 of `asset_id`
    std::string_view asset_id;   // e.g. "Pl0500"
    std::string_view name;       // e.g. "Eugen"
    Address          vftable_rva; // vftable that the entity uses (RVA against
                                 // the design-time image base 0x140000000).
                                 // 0 if not statically known.
    bool             is_playable = true;
};

inline constexpr std::array<CharacterTypeEntry, 32> kCharacterTypes = {{
    {0x26A4848A, "Pl0000", "Gran",                    0x14616DCA8ULL},
    {0x9498420D, "Pl0100", "Djeeta",                  0x14616E7A8ULL},
    {0x34D4FD8F, "Pl0200", "Katalina",                0x145CAE340ULL},
    {0xF8D73D33, "Pl0300", "Rackam",                  0x145C8E3E0ULL},
    {0x7B5934AD, "Pl0400", "Io",                      0x145CB40F0ULL},
    {0x443D46BB, "Pl0500", "Eugen",                   0x145AEE940ULL},
    {0xA9D6569E, "Pl0600", "Rosetta",                 0x145CBCBE0ULL},
    {0xFBA6615D, "Pl0700", "Ferry",                   0x145CC5220ULL},
    {0x63A7C3F0, "Pl0800", "Lancelot",                0x145CCD780ULL},
    {0xF96A90C2, "Pl0900", "Vane",                    0x145CD5DE0ULL},
    {0x28AC1108, "Pl1000", "Percival",                0x145CDE310ULL},
    {0x94E2514E, "Pl1100", "Siegfried",               0x145AE36C0ULL},
    {0x2B4AA114, "Pl1200", "Charlotta",               0x145CE6AD0ULL},
    {0xC97F3365, "Pl1300", "Yodarha",                 0x145CEF030ULL},
    {0x601AA977, "Pl1400", "Narmaya",                 0x145CF7ED0ULL},
    {0xBCC238DE, "Pl1500", "Ghandagoza",              0x145D00620ULL},
    {0xC3155079, "Pl1600", "Zeta",                    0x145D08CB0ULL},
    {0xD16CFBDE, "Pl1700", "Vaseraga",                0x145AD28A0ULL},
    {0x6FDD6932, "Pl1800", "Cagliostro",              0x1459E9D50ULL},
    {0x8056ABCD, "Pl1900", "Id",                      0x14617F198ULL},
    {0xF5755C0E, "Pl2000", "Id (Transformed)",        0x1461807B8ULL},
    {0x9C89A455, "Pl2100", "Sandalphon",              0x145B251B0ULL},
    {0x59DB0CD9, "Pl2200", "Seofon",                  0x145D11F80ULL},
    {0xDA5A8E25, "Pl2300", "Tweyen",                  0x145D1AE80ULL},
    {0x4C714F77, "Pl2400", "Gallanza",                 0x145D235A0ULL},
    {0xE330418F, "Pl2500", "Maglielle",                0x145D2C500ULL},
    {0xE3D1BE26, "Pl2600", "Beatrix",                  0x145D34E90ULL},
    {0x91418145, "Pl2700", "Eustace",                  0x145D3D720ULL},
    {0x48ADDA36, "Pl2800", "Fraux",                    0x145D47370ULL},
    {0x0A58FB4D, "Pl2900", "Fediel",                   0x145ADAF50ULL},
    {0x2AF678E8, "Pl0700Ghost",          "Ferry Ghost",            0x1459D3080ULL, false},
    {0x8364C8BC, "Pl0700GhostSatellite", "Ferry Ghost (Satellite)",0x1459D3F90ULL, false},
}};

// Compile-time guard: each table row must reproduce when hashed.
namespace detail {
template <std::size_t I>
constexpr bool character_table_row_ok() noexcept {
    constexpr auto row = kCharacterTypes[I];
    return StringHash32(row.asset_id).value() == row.hash;
}
template <std::size_t... I>
constexpr bool character_table_all_ok(std::index_sequence<I...>) noexcept {
    return ((character_table_row_ok<I>()) && ...);
}
} // namespace detail

static_assert(detail::character_table_all_ok(
    std::make_index_sequence<kCharacterTypes.size()>{}),
    "gbfr::kCharacterTypes: hash recomputation mismatch — "
    "either an entry was mistyped or StringHash32 is broken.");

// Look up by hash. Returns nullopt for unknown hashes.
[[nodiscard]] constexpr std::optional<CharacterTypeEntry>
find_character_type(std::uint32_t hash) noexcept {
    for (const auto& row : kCharacterTypes) {
        if (row.hash == hash) return row;
    }
    return std::nullopt;
}

// Look up by entity vftable RVA. Returns nullopt for any vftable not in the
// player set.
[[nodiscard]] constexpr std::optional<CharacterTypeEntry>
find_character_type_by_vftable(Address vftable_rva) noexcept {
    for (const auto& row : kCharacterTypes) {
        if (row.vftable_rva == vftable_rva) return row;
    }
    return std::nullopt;
}

} // namespace gbfr
