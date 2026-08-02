#!/usr/bin/env python3
"""Refresh generated item, sigil, trait, summon, and summon-stat catalogs."""

from __future__ import annotations

import argparse
import json
import re
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ITEM_CPP = ROOT / "src" / "sdk" / "src" / "item_types.cpp"
SKILL_CPP = ROOT / "src" / "sdk" / "src" / "skill_types.cpp"
SUMMON_CPP = ROOT / "src" / "sdk" / "src" / "summon_types.cpp"

DEFAULT_REPOSITORY = "villith/relink-logs"
DEFAULT_REF = "c1b5a32d6671ff4d710bfe2a228a70c20351311f"

ENTRY_RE = re.compile(
    r'\{0x([0-9A-Fa-f]+),\s*"((?:\\.|[^"])*)",\s*"((?:\\.|[^"])*)"\}'
)

# These live ItemList entries have structured engine IDs but no published
# English display name. Showing the asset ID is safer than inventing a name.
SUPPLEMENTAL_ITEM_IDS = (
    "ITEM_13_0004",
    "ITEM_24_0000",
    "ITEM_24_0001",
    "ITEM_24_1000",
    "ITEM_24_1001",
    "ITEM_24_1002",
    "ITEM_24_1003",
    "ITEM_33_0001",
    "ITEM_33_0004",
)


def rotate_left(value: int, bits: int) -> int:
    return ((value << bits) | (value >> (32 - bits))) & 0xFFFFFFFF


def game_hash(text: str) -> int:
    data = text.encode("utf-8")
    p1 = 0x9E3779B1
    p2 = 0x85EBCA77
    p3 = 0xC2B2AE3D
    p4 = 0x27D4EB2F
    p5 = 0x165667B1

    def read_u32(offset: int) -> int:
        return int.from_bytes(data[offset : offset + 4], "little")

    def round_value(accumulator: int, value: int) -> int:
        return rotate_left((accumulator + value * p2) & 0xFFFFFFFF, 13) * p1 & 0xFFFFFFFF

    index = 0
    length = len(data)
    result = 0x178A54A4
    if length >= 16:
        lanes = [0x2557311B, 0x871FB76A, 0x0133ECF3, 0x62FC7342]
        limit = length - 16
        while True:
            for lane in range(4):
                lanes[lane] = round_value(lanes[lane], read_u32(index + lane * 4))
            index += 16
            if index > limit:
                break
        result = (
            rotate_left(lanes[0], 1)
            + rotate_left(lanes[1], 7)
            + rotate_left(lanes[2], 12)
            + rotate_left(lanes[3], 18)
        ) & 0xFFFFFFFF

    result = (result + length) & 0xFFFFFFFF
    while index + 4 <= length:
        result = rotate_left((result + read_u32(index) * p3) & 0xFFFFFFFF, 17) * p4 & 0xFFFFFFFF
        index += 4
    while index < length:
        result = rotate_left((result + data[index] * p5) & 0xFFFFFFFF, 11) * p1 & 0xFFFFFFFF
        index += 1

    result ^= result >> 15
    result = result * p2 & 0xFFFFFFFF
    result ^= result >> 13
    result = result * p3 & 0xFFFFFFFF
    result ^= result >> 16
    return result & 0xFFFFFFFF


def cpp_unescape(value: str) -> str:
    return value.replace(r"\\", "\\").replace(r"\"", '"')


def cpp_escape(value: str) -> str:
    return value.replace("\\", r"\\").replace('"', r"\"").replace("\n", r"\n")


def parse_existing(path: Path) -> dict[int, tuple[str, str]]:
    entries: dict[int, tuple[str, str]] = {}
    for match in ENTRY_RE.finditer(path.read_text(encoding="utf-8")):
        hash_value = int(match.group(1), 16)
        entry = (cpp_unescape(match.group(2)), cpp_unescape(match.group(3)))
        if entry[0].startswith(("ITEM_", "GEEN_", "SKILL_")):
            computed = game_hash(entry[0])
            if computed != hash_value:
                print(
                    f"dropping invalid legacy row {entry[0]}: "
                    f"0x{hash_value:08X} != 0x{computed:08X}"
                )
                continue
        previous = entries.get(hash_value)
        if previous is not None and previous != entry:
            raise ValueError(f"conflicting existing hash 0x{hash_value:08X}: {previous} vs {entry}")
        entries[hash_value] = entry
    return entries


def fetch_url_json(url: str) -> dict:
    with urllib.request.urlopen(url) as response:
        return json.load(response)


def fetch_json(repository: str, ref: str, filename: str) -> dict[str, dict[str, str]]:
    return fetch_url_json(
        f"https://raw.githubusercontent.com/{repository}/{ref}/"
        f"src-tauri/lang/en/{filename}"
    )


def load_catalog(repository: str, ref: str, filenames: tuple[str, ...]) -> dict[int, tuple[str, str]]:
    entries: dict[int, tuple[str, str]] = {}
    for filename in filenames:
        for hash_text, row in fetch_json(repository, ref, filename).items():
            hash_value = int(hash_text, 16)
            asset_id = row["key"]
            name = row["text"]
            computed = game_hash(asset_id)
            is_raw_hash_id = (
                re.fullmatch(r"[0-9A-Fa-f]{8}", asset_id) is not None
                and int(asset_id, 16) == hash_value
            )
            if computed != hash_value and not is_raw_hash_id:
                raise ValueError(
                    f"{filename}: {asset_id} hashes to 0x{computed:08X}, "
                    f"catalog says 0x{hash_value:08X}"
                )
            previous = entries.get(hash_value)
            entry = (asset_id, name)
            if previous is not None and previous != entry:
                raise ValueError(f"conflicting upstream hash 0x{hash_value:08X}: {previous} vs {entry}")
            entries[hash_value] = entry
    return entries


def merge_catalog(
    existing: dict[int, tuple[str, str]],
    upstream: dict[int, tuple[str, str]],
) -> dict[int, tuple[str, str]]:
    merged = dict(existing)
    merged.update(upstream)
    return merged


def item_sort_key(entry: tuple[int, tuple[str, str]]) -> tuple[int, str, int]:
    hash_value, (asset_id, _) = entry
    if asset_id.startswith("ITEM_"):
        group = 0
    elif asset_id.startswith("GEEN_"):
        group = 1
    else:
        group = 2
    return group, asset_id, hash_value


def render_items(
    entries: dict[int, tuple[str, str]],
    repository: str,
    ref: str,
) -> str:
    rows = []
    for hash_value, (asset_id, name) in sorted(entries.items(), key=item_sort_key):
        rows.append(
            f'    {{0x{hash_value:08X}, "{cpp_escape(asset_id)}", "{cpp_escape(name)}"}},'
        )
    body = "\n".join(rows)
    return f"""// SPDX-License-Identifier: MIT
// Generated from {repository} English game-data catalogs at {ref}.
// Legacy-only hashes are retained for compatibility.

#include "gbfr/item_types.hpp"

namespace gbfr {{

namespace {{

constexpr ItemTypeEntry kItemTypes[] = {{
{body}
}};

constexpr bool item_hashes_are_valid() {{
    for (const auto& entry : kItemTypes) {{
        const bool structured_id =
            entry.asset_id.starts_with("ITEM_") ||
            entry.asset_id.starts_with("GEEN_");
        if (structured_id &&
            StringHash32(entry.asset_id).value() != entry.hash) {{
            return false;
        }}
    }}
    return true;
}}

static_assert(item_hashes_are_valid(), "item catalog hash mismatch");

}} // namespace

const ItemTypeEntry* find_item_type(std::uint32_t hash) noexcept {{
    for (const auto& entry : kItemTypes) {{
        if (entry.hash == hash) return &entry;
    }}
    return nullptr;
}}

}} // namespace gbfr
"""


def render_skills(
    entries: dict[int, tuple[str, str]],
    repository: str,
    ref: str,
) -> str:
    rows = []
    for hash_value, (asset_id, name) in sorted(
        entries.items(), key=lambda item: (item[1][0], item[0])
    ):
        rows.append(
            f'    {{0x{hash_value:08X}, "{cpp_escape(asset_id)}", "{cpp_escape(name)}"}},'
        )
    body = "\n".join(rows)
    return f"""// SPDX-License-Identifier: MIT
// Generated from {repository} English game-data catalogs at {ref}.
// Legacy-only hashes are retained for compatibility.

#include "gbfr/skill_types.hpp"

#include <cctype>

namespace gbfr {{

namespace {{

constexpr SkillEntry kSkills[] = {{
{body}
}};

constexpr bool skill_hashes_are_valid() {{
    for (const auto& entry : kSkills) {{
        if (StringHash32(entry.asset_id).value() != entry.hash) return false;
    }}
    return true;
}}

static_assert(skill_hashes_are_valid(), "skill catalog hash mismatch");

}} // namespace

const SkillEntry* find_skill(std::uint32_t hash) noexcept {{
    for (const auto& entry : kSkills) {{
        if (entry.hash == hash) return &entry;
    }}
    return nullptr;
}}

bool ascii_iequals(std::string_view left, std::string_view right) {{
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {{
        const auto lower_a = static_cast<char>(
            std::tolower(static_cast<unsigned char>(left[i])));
        const auto lower_b = static_cast<char>(
            std::tolower(static_cast<unsigned char>(right[i])));
        if (lower_a != lower_b) return false;
    }}
    return true;
}}

const SkillEntry* find_skill_by_name(
    std::string_view value, std::uint32_t preferred_hash) noexcept {{
    if (value.empty()) return nullptr;

    for (const auto& entry : kSkills) {{
        if (ascii_iequals(entry.asset_id, value)) return &entry;
    }}

    const SkillEntry* match = nullptr;
    bool ambiguous = false;
    for (const auto& entry : kSkills) {{
        if (!ascii_iequals(entry.name, value)) continue;
        if (entry.hash == preferred_hash) return &entry;
        if (match == nullptr) match = &entry;
        else ambiguous = true;
    }}
    return ambiguous ? nullptr : match;
}}

const SkillEntry* find_skill_by_name(std::string_view value) noexcept {{
    return find_skill_by_name(value, 0);
}}

}} // namespace gbfr
"""


def render_summons(
    summons: dict[str, dict[str, str]],
    bonuses: dict[str, dict[str, str]],
    bonus_values: dict[str, dict],
    repository: str,
    ref: str,
) -> str:
    summon_rows = []
    for hash_text, row in sorted(summons.items()):
        summon_rows.append(
            f'    {{0x{int(hash_text, 16):08X}, '
            f'"{cpp_escape(row["key"])}", "{cpp_escape(row["text"])}"}},'
        )

    if set(bonuses) != set(bonus_values):
        missing_values = sorted(set(bonuses) - set(bonus_values))
        missing_names = sorted(set(bonus_values) - set(bonuses))
        raise ValueError(
            f"summon bonus catalog mismatch: "
            f"missing values={missing_values}, missing names={missing_names}"
        )

    bonus_rows = []
    for hash_text, row in sorted(bonuses.items()):
        value_row = bonus_values[hash_text]
        values = value_row["values"]
        if len(values) != 10:
            raise ValueError(f"summon bonus {hash_text} has {len(values)} values")
        value_list = ", ".join(str(int(value)) for value in values)
        percent = "true" if value_row["percent"] else "false"
        bonus_rows.append(
            f'    {{0x{int(hash_text, 16):08X}, '
            f'"{cpp_escape(row["key"])}", "{cpp_escape(row["text"])}", '
            f'{{{{{value_list}}}}}, {percent}}},'
        )

    return f"""// SPDX-License-Identifier: MIT
// Generated from {repository} summon catalogs at {ref}.

#include "gbfr/summon_types.hpp"

namespace gbfr {{

namespace {{

constexpr SummonTypeEntry kSummonTypes[] = {{
{chr(10).join(summon_rows)}
}};

constexpr SummonStatTypeEntry kSummonStatTypes[] = {{
{chr(10).join(bonus_rows)}
}};

constexpr std::size_t kSummonStatTypeCount =
    sizeof(kSummonStatTypes) / sizeof(kSummonStatTypes[0]);

}} // namespace

const SummonTypeEntry* find_summon_type(std::uint32_t hash) noexcept {{
    for (const auto& entry : kSummonTypes) {{
        if (entry.hash == hash) return &entry;
    }}
    return nullptr;
}}

const SummonStatTypeEntry* find_summon_stat_type(std::uint32_t hash) noexcept {{
    for (const auto& entry : kSummonStatTypes) {{
        if (entry.hash == hash) return &entry;
    }}
    return nullptr;
}}

std::size_t summon_stat_type_count() noexcept {{
    return kSummonStatTypeCount;
}}

const SummonStatTypeEntry* summon_stat_type_at(std::size_t index) noexcept {{
    return index < kSummonStatTypeCount ? &kSummonStatTypes[index] : nullptr;
}}

}} // namespace gbfr
"""


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", default=DEFAULT_REPOSITORY)
    parser.add_argument("--ref", default=DEFAULT_REF)
    args = parser.parse_args()

    existing_items = parse_existing(ITEM_CPP)
    existing_skills = parse_existing(SKILL_CPP)
    upstream_items = load_catalog(args.repository, args.ref, ("items.json", "sigils.json"))
    upstream_skills = load_catalog(args.repository, args.ref, ("traits.json",))
    summons = fetch_json(args.repository, args.ref, "summons.json")
    summon_bonuses = fetch_json(args.repository, args.ref, "summon-bonuses.json")
    summon_bonus_values = fetch_url_json(
        f"https://raw.githubusercontent.com/{args.repository}/{args.ref}/"
        "src-tauri/assets/summon-bonus-values.json"
    )

    items = merge_catalog(existing_items, upstream_items)
    for asset_id in SUPPLEMENTAL_ITEM_IDS:
        items.setdefault(game_hash(asset_id), (asset_id, asset_id))
    skills = merge_catalog(existing_skills, upstream_skills)

    ITEM_CPP.write_text(
        render_items(items, args.repository, args.ref),
        encoding="utf-8",
        newline="\n",
    )
    SKILL_CPP.write_text(
        render_skills(skills, args.repository, args.ref),
        encoding="utf-8",
        newline="\n",
    )
    SUMMON_CPP.write_text(
        render_summons(
            summons,
            summon_bonuses,
            summon_bonus_values,
            args.repository,
            args.ref,
        ),
        encoding="utf-8",
        newline="\n",
    )

    print(f"wrote {len(items)} item/sigil rows")
    print(f"wrote {len(skills)} trait rows")
    print(f"wrote {len(summons)} summon rows")
    print(f"wrote {len(summon_bonuses)} summon stat rows")


if __name__ == "__main__":
    main()
