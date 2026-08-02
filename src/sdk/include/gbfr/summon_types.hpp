#pragma once
// Generated summon and summon-stat metadata for game 2.0.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace gbfr {

struct SummonTypeEntry {
    std::uint32_t    hash;
    std::string_view asset_id;
    std::string_view name;
};

struct SummonStatTypeEntry {
    std::uint32_t    hash;
    std::string_view asset_id;
    std::string_view name;
    std::array<std::uint32_t, 10> values;
    bool is_percent;
};

const SummonTypeEntry* find_summon_type(std::uint32_t hash) noexcept;
const SummonStatTypeEntry* find_summon_stat_type(std::uint32_t hash) noexcept;
std::size_t summon_stat_type_count() noexcept;
const SummonStatTypeEntry* summon_stat_type_at(std::size_t index) noexcept;

} // namespace gbfr
