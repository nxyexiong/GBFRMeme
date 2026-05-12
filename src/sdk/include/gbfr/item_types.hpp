#pragma once
// Generated from Nenkai's modding docs (https://nenkai.github.io/relink-modding/resources/item_ids/).
// Hash -> { asset_id, name } for every known item in the cyan engine.
//
// The table is sorted by hash. Lookup is via linear search at runtime —
// a few hundred entries, fits in a single L1 cache line walk per call.
//
// Add new rows when the modding docs update; the file is hand-curated for now.

#include "common.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace gbfr {

struct ItemTypeEntry {
    std::uint32_t    hash;
    std::string_view asset_id;
    std::string_view name;
};

// Look up an item by its `string_hash32`. Returns nullptr for unknown hashes.
const ItemTypeEntry* find_item_type(std::uint32_t hash) noexcept;

} // namespace gbfr
