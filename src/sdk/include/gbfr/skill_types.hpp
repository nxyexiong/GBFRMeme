#pragma once
// Skill/trait id table.
// Source: https://nenkai.github.io/relink-modding/resources/trait_skill_ids/
//
// Sigil entries carry two trait_ids; each trait_id is a `string_hash32` of
// a `SKILL_xxx_xx` asset id, NOT a `GEEN_xxx_xx` id. The two namespaces
// differ — `GEEN` is the *sigil template* the trait was rolled on (which we
// already cover in item_types.cpp), while `SKILL` is the *underlying effect*
// the trait grants.

#include "common.hpp"

#include <cstdint>
#include <string_view>

namespace gbfr {

struct SkillEntry {
    std::uint32_t    hash;
    std::string_view asset_id;
    std::string_view name;
};

// Look up a skill/trait by `string_hash32`. Returns nullptr if unknown.
const SkillEntry* find_skill(std::uint32_t hash) noexcept;

// Look up a skill/trait by exact display name (case-insensitive). Returns
// nullptr if no skill has that name. Used by setter APIs that take a
// human-readable trait name.
const SkillEntry* find_skill_by_name(std::string_view name) noexcept;

} // namespace gbfr
