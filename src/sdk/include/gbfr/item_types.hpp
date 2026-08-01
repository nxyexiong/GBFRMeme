#pragma once
// Generated hash -> { asset_id, name } catalog for items, sigils, and
// wrightstone templates. Refresh with `python scripts/update_catalogs.py`.

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
