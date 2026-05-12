// SPDX-License-Identifier: MIT
// gbfr/character_types.hpp — playable-character hash table.
//
// Maps the engine's `string_hash32` of each character's actor id (e.g.
// `"Pl0000"` -> `0x26A4848A`) to its asset prefix and human-readable name.
//
// Source: cross-referenced from
//   - false-spring/gbfr-logs: src-tauri/src/parser/constants.rs
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
};

inline constexpr std::array<CharacterTypeEntry, 26> kCharacterTypes = {{
    {0x26A4848A, "Pl0000", "Gran",                    0x14517FA30ULL},
    {0x9498420D, "Pl0100", "Djeeta",                  0x1451802D0ULL},
    {0x34D4FD8F, "Pl0200", "Katalina",                0x144DE27D8ULL},
    {0xF8D73D33, "Pl0300", "Rackam",                  0x144DBB4B8ULL},
    {0x7B5934AD, "Pl0400", "Io",                      0x144DE76B8ULL},
    {0x443D46BB, "Pl0500", "Eugen",                   0x144C94368ULL},
    {0xA9D6569E, "Pl0600", "Rosetta",                 0x144DEEFC8ULL},
    {0xFBA6615D, "Pl0700", "Ferry",                   0x144DF6498ULL},
    {0x63A7C3F0, "Pl0800", "Lancelot",                0x144DFD988ULL},
    {0xF96A90C2, "Pl0900", "Vane",                    0x144E04ED8ULL},
    {0x28AC1108, "Pl1000", "Percival",                0x144E0C358ULL},
    {0x94E2514E, "Pl1100", "Siegfried",               0x144C891C8ULL},
    {0x2B4AA114, "Pl1200", "Charlotta",               0x144E138C8ULL},
    {0xC97F3365, "Pl1300", "Yodarha",                 0x144E1ACD8ULL},
    {0x601AA977, "Pl1400", "Narmaya",                 0x144DC58E8ULL},
    {0xBCC238DE, "Pl1500", "Ghandagoza",              0x144E22A48ULL},
    {0xC3155079, "Pl1600", "Zeta",                    0x144E29EE8ULL},
    {0xD16CFBDE, "Pl1700", "Vaseraga",                0x144C81BF8ULL},
    {0x6FDD6932, "Pl1800", "Cagliostro",              0x144BC9EA8ULL},
    {0x8056ABCD, "Pl1900", "Id",                      0x1451584F0ULL},
    {0xF5755C0E, "Pl2000", "Id (Transformed)",        0x145159630ULL},
    {0x9C89A455, "Pl2100", "Sandalphon",              0x144CBBE38ULL},
    {0x59DB0CD9, "Pl2200", "Seofon",                  0x144E321E8ULL},
    {0xDA5A8E25, "Pl2300", "Tweyen",                  0x144E3A148ULL},
    {0x2AF678E8, "Pl0700Ghost",          "Ferry Ghost",            0x144BB59F8ULL},
    {0x8364C8BC, "Pl0700GhostSatellite", "Ferry Ghost (Satellite)",0x144BB6888ULL},
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
