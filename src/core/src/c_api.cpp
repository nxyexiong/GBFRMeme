// SPDX-License-Identifier: MIT
#include "gbfr/core/c_api.h"

#include "gbfr/character_types.hpp"
#include "gbfr/core/debug.hpp"
#include "gbfr/core/session.hpp"
#include "gbfr/core/ui_runtime.hpp"
#include "gbfr/item_types.hpp"
#include "gbfr/save/sys_data_lists.hpp"
#include "gbfr/signatures.hpp"
#include "gbfr/skill_types.hpp"

#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace {

std::mutex                              g_mutex;
std::unique_ptr<gbfr::core::Session>    g_session;
std::unique_ptr<gbfr::core::UiRuntime>  g_ui;

std::unique_ptr<gbfr::core::Session> make_session(GbfrAttachMode mode, const wchar_t* exe) {
    if (mode == GBFR_ATTACH_INTERNAL) {
        auto s = gbfr::core::Session::attach_in_process();
        return s.has_value()
            ? std::make_unique<gbfr::core::Session>(std::move(*s))
            : nullptr;
    }
    std::wstring name = (exe != nullptr) ? std::wstring{exe} : std::wstring{};
    auto s = name.empty()
        ? gbfr::core::Session::attach_external_by_name()
        : gbfr::core::Session::attach_external_by_name(name);
    return s.has_value()
        ? std::make_unique<gbfr::core::Session>(std::move(*s))
        : nullptr;
}

bool require_session() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return static_cast<bool>(g_session);
}

// Acquire a borrowed Session pointer (or nullptr). Caller must hold no
// other lock and must not retain the pointer past the call.
gbfr::core::Session* borrow_session_or_null() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_session.get();
}

template <class T>
GbfrStatus dbg_read_typed(uint64_t addr, T* out) {
    if (!out) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    auto v = gbfr::core::debug::read_typed<T>(*s, addr);
    if (!v) return GBFR_ERR_PLATFORM;
    *out = *v;
    return GBFR_OK;
}

GbfrStatus copy_addr_vec(const std::vector<gbfr::Address>& v,
                         uint64_t* out_addrs, uint32_t cap, uint32_t* out_count) {
    if (!out_count) return GBFR_ERR_INVALID_ARG;
    *out_count = static_cast<uint32_t>(v.size());
    if (cap > 0 && out_addrs) {
        const uint32_t n = std::min<uint32_t>(cap, static_cast<uint32_t>(v.size()));
        for (uint32_t i = 0; i < n; ++i) out_addrs[i] = static_cast<uint64_t>(v[i]);
    }
    return v.empty() ? GBFR_ERR_NOT_FOUND : GBFR_OK;
}

} // namespace

// ---------------------------------------------------------------------------
// 1. Lifecycle
// ---------------------------------------------------------------------------

extern "C" {

GBFR_CORE_API uint32_t GBFR_CORE_CALL gbfr_api_version(void) {
    return 2u;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_init(GbfrAttachMode mode,
                                                  const wchar_t* exe,
                                                  uint32_t       flags) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_session) return GBFR_ERR_ALREADY_INIT;
    auto s = make_session(mode, exe);
    if (!s) return GBFR_ERR_NOT_FOUND;
    g_session = std::move(s);

    if ((flags & GBFR_INIT_NO_UI) == 0u) {
        g_ui = std::make_unique<gbfr::core::UiRuntime>();
        g_ui->start(mode);
    }
    return GBFR_OK;
}

GBFR_CORE_API void GBFR_CORE_CALL gbfr_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_ui) {
        g_ui->stop();
        g_ui.reset();
    }
    g_session.reset();
}

GBFR_CORE_API int GBFR_CORE_CALL gbfr_is_initialized(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_session ? 1 : 0;
}

GBFR_CORE_API void GBFR_CORE_CALL gbfr_wait_for_exit(void) {
    // Take a borrowed pointer under the lock, then release the lock before
    // blocking — otherwise `gbfr_shutdown` from another thread would deadlock.
    gbfr::core::UiRuntime* ui = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        ui = g_ui.get();
    }
    if (ui) ui->wait_for_exit();
}

// ---------------------------------------------------------------------------
// 2. CharacterManager
//    All getters return GBFR_ERR_NOT_AVAILABLE until the SDK's
//    SaveListBase / CharaData offsets and CharacterManager singleton
//    resolution are wired up.
// ---------------------------------------------------------------------------

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_character_get_count(uint32_t* out_count) {
    if (!out_count) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;

    const auto agg = s->save_aggregate_address();
    if (agg == 0) { *out_count = 0; return GBFR_ERR_NOT_FOUND; }

    gbfr::sys::data::CharaList list(s->memory(),
        agg + static_cast<gbfr::Address>(gbfr::signatures::offset::save_aggregate::kCharaList));
    *out_count = list.live_count();
    return GBFR_OK;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_character_get_at(uint32_t index,
                                                              GbfrCharacterInfo* out_info) {
    if (!out_info) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    std::memset(out_info, 0, sizeof(*out_info));

    const auto agg = s->save_aggregate_address();
    if (agg == 0) return GBFR_ERR_NOT_FOUND;

    gbfr::sys::data::CharaList list(s->memory(),
        agg + static_cast<gbfr::Address>(gbfr::signatures::offset::save_aggregate::kCharaList));

    // Walk the array, skipping empty slots, and return the `index`-th live entry.
    // Note: the dword at entry+0x10 is NOT the character_type hash. It's an
    // engine-internal per-save record id whose meaning is not yet recovered.
    // We deliberately leave `name_id` / `display_name` blank here; the
    // character_type comes from the live Entity (combat path), not the save.
    std::uint32_t live = 0;
    for (std::size_t i = 0; i < list.kMaxCount; ++i) {
        if (!list.is_live(i)) continue;
        if (live == index) {
            // Stash the opaque key in costume_id for diagnostic visibility.
            const auto key = list.key_at(i);
            std::snprintf(out_info->costume_id, sizeof(out_info->costume_id),
                          "#%08x", key.value());
            return GBFR_OK;
        }
        ++live;
    }
    return GBFR_ERR_OUT_OF_RANGE;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_character_get_by_name(const char* name_id,
                                                                   GbfrCharacterInfo* out_info) {
    if (!name_id || !out_info) return GBFR_ERR_INVALID_ARG;
    if (!require_session()) return GBFR_ERR_NOT_INIT;
    std::memset(out_info, 0, sizeof(*out_info));
    return GBFR_ERR_NOT_AVAILABLE;
}

// ---------------------------------------------------------------------------
// 3. Live combat
//    Returns GBFR_ERR_NOT_AVAILABLE until the per-entity offsets and the
//    local-slot pointer are recovered.
// ---------------------------------------------------------------------------

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_combat_get_current_info(GbfrCombatInfo* out_info) {
    if (!out_info) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    std::memset(out_info, 0, sizeof(*out_info));

    const gbfr::Address entity = s->find_local_player();
    if (entity == 0) return GBFR_ERR_NOT_FOUND;

    // Re-read the vftable to resolve character_type.
    std::uint64_t vft = 0;
    if (!s->memory().read(entity, &vft, sizeof(vft))) return GBFR_ERR_PLATFORM;
    for (const auto& row : gbfr::kCharacterTypes) {
        if (s->rva(row.vftable_rva) == static_cast<gbfr::Address>(vft)) {
            const auto n_id = std::min<std::size_t>(63, row.asset_id.size());
            const auto n_nm = std::min<std::size_t>(63, row.name.size());
            std::memcpy(out_info->character_name_id, row.asset_id.data(), n_id);
            std::memcpy(out_info->character_display_name, row.name.data(), n_nm);
            break;
        }
    }

    return gbfr_combat_read_from_entity(static_cast<uint64_t>(entity), out_info);
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_combat_read_from_entity(
    uint64_t entity_addr, GbfrCombatInfo* out_info) {
    if (!out_info || entity_addr == 0) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;

    const auto pdo = s->player_data_offset();
    if (pdo == 0) return GBFR_ERR_NOT_FOUND;

    // PlayerStats layout (from gbfr-logs `hooks::ffi::PlayerStats`):
    //   +0x00 u32 level
    //   +0x04 u32 total_health
    //   +0x08 u32 total_attack
    //   +0x0C u32 unk
    //   +0x10 f32 stun_power
    //   +0x14 f32 critical_rate
    //   +0x18 u32 total_power
    struct PlayerStatsRaw {
        std::uint32_t level;
        std::uint32_t total_health;
        std::uint32_t total_attack;
        std::uint32_t unk;
        float         stun_power;
        float         critical_rate;
        std::uint32_t total_power;
    };
    PlayerStatsRaw stats{};
    if (!s->memory().read(static_cast<gbfr::Address>(entity_addr) + pdo,
                          &stats, sizeof(stats))) {
        return GBFR_ERR_PLATFORM;
    }

    out_info->level  = stats.level;
    out_info->hp     = static_cast<float>(stats.total_health);
    out_info->hp_max = static_cast<float>(stats.total_health);
    out_info->critical_rate = stats.critical_rate;

    return GBFR_OK;
}

// ---------------------------------------------------------------------------
// 5. Inventory (sys::data::ItemList walking)
// ---------------------------------------------------------------------------

namespace {

// ItemData entry layout (stride 0x20, verified live):
//   +0x00 u32 item_hash    (string_hash32; 0x887AE0B0 = hash("") = empty)
//   +0x04 u32 count        (quantity owned)
//   +0x08 u32 type_tag     (0x04 or 0x0C observed)
//   +0x0C u32 acquired_seq (acquisition order / timestamp)
//   +0x10..+0x1F padding
struct ItemDataRaw {
    std::uint32_t item_hash;
    std::uint32_t count;
    std::uint32_t type_tag;
    std::uint32_t acquired_seq;
    std::uint32_t pad0;
    std::uint32_t pad1;
    std::uint32_t pad2;
    std::uint32_t pad3;
};
static_assert(sizeof(ItemDataRaw) == gbfr::signatures::offset::item_list::kEntryStride,
              "ItemData stride mismatch");

constexpr std::uint32_t kItemListMaxCount      = gbfr::signatures::count::kItemListMax;
constexpr std::uint32_t kItemListEntriesOffset =
    static_cast<std::uint32_t>(gbfr::signatures::offset::item_list::kEntriesOffset);
constexpr std::uint32_t kEmptyKeyHash          = gbfr::signatures::sentinel::kEmptyKey;

} // namespace

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_item_get_count(uint32_t* out_count) {
    if (!out_count) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;

    const gbfr::Address list = s->find_save_list(gbfr::signatures::vft::kItemList);
    if (list == 0) { *out_count = 0; return GBFR_ERR_NOT_FOUND; }

    std::uint32_t live = 0;
    for (std::uint32_t i = 0; i < kItemListMaxCount; ++i) {
        const auto addr = list + kItemListEntriesOffset
                        + static_cast<gbfr::Address>(i) * sizeof(ItemDataRaw);
        std::uint32_t key = 0;
        if (!s->memory().read(addr, &key, sizeof(key))) continue;
        if (key != 0 && key != kEmptyKeyHash) ++live;
    }
    *out_count = live;
    return GBFR_OK;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_item_get_at(uint32_t index,
                                                         GbfrItemInfo* out_info) {
    if (!out_info) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    std::memset(out_info, 0, sizeof(*out_info));

    const gbfr::Address list = s->find_save_list(gbfr::signatures::vft::kItemList);
    if (list == 0) return GBFR_ERR_NOT_FOUND;

    std::uint32_t live = 0;
    for (std::uint32_t i = 0; i < kItemListMaxCount; ++i) {
        const auto addr = list + kItemListEntriesOffset
                        + static_cast<gbfr::Address>(i) * sizeof(ItemDataRaw);
        ItemDataRaw raw{};
        if (!s->memory().read(addr, &raw, sizeof(raw))) continue;
        if (raw.item_hash == 0 || raw.item_hash == kEmptyKeyHash) continue;
        if (live == index) {
            out_info->item_hash    = raw.item_hash;
            out_info->count        = raw.count;
            out_info->acquired_seq = raw.acquired_seq;
            out_info->type_tag     = raw.type_tag;
            if (const auto* row = gbfr::find_item_type(raw.item_hash)) {
                const auto n_id = std::min<std::size_t>(63, row->asset_id.size());
                const auto n_nm = std::min<std::size_t>(63, row->name.size());
                std::memcpy(out_info->asset_id,     row->asset_id.data(), n_id);
                std::memcpy(out_info->display_name, row->name.data(),     n_nm);
            }
            return GBFR_OK;
        }
        ++live;
    }
    return GBFR_ERR_OUT_OF_RANGE;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_item_set_count(uint32_t index,
                                                            uint32_t count) {
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;

    const gbfr::Address list = s->find_save_list(gbfr::signatures::vft::kItemList);
    if (list == 0) return GBFR_ERR_NOT_FOUND;

    std::uint32_t live = 0;
    for (std::uint32_t i = 0; i < kItemListMaxCount; ++i) {
        const auto addr = list + kItemListEntriesOffset
                        + static_cast<gbfr::Address>(i) * sizeof(ItemDataRaw);
        std::uint32_t key = 0;
        if (!s->memory().read(addr, &key, sizeof(key))) continue;
        if (key == 0 || key == kEmptyKeyHash) continue;
        if (live == index) {
            const auto count_addr = addr + offsetof(ItemDataRaw, count);
            if (!s->memory().write(count_addr, &count, sizeof(count))) {
                return GBFR_ERR_PLATFORM;
            }
            return GBFR_OK;
        }
        ++live;
    }
    return GBFR_ERR_OUT_OF_RANGE;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_item_lookup(
    uint32_t hash, char* out_asset_id, char* out_name) {
    const auto* row = gbfr::find_item_type(hash);
    if (!row) {
        if (out_asset_id) out_asset_id[0] = '\0';
        if (out_name)     out_name[0]     = '\0';
        return GBFR_ERR_NOT_FOUND;
    }
    if (out_asset_id) {
        const auto n = std::min<std::size_t>(63, row->asset_id.size());
        std::memcpy(out_asset_id, row->asset_id.data(), n);
        out_asset_id[n] = '\0';
    }
    if (out_name) {
        const auto n = std::min<std::size_t>(63, row->name.size());
        std::memcpy(out_name, row->name.data(), n);
        out_name[n] = '\0';
    }
    return GBFR_OK;
}

// ---------------------------------------------------------------------------
// 6. Sigils (sys::data::GemList walking)
// ---------------------------------------------------------------------------

namespace {

// GemList layout (verified live):
//   +0x00 vftable
//   +0x08 entries[N], stride 0x24 (matches gbfr-logs SigilEntry).
struct SigilEntryRaw {
    std::uint32_t first_trait_id;
    std::uint32_t first_trait_level;
    std::uint32_t second_trait_id;
    std::uint32_t second_trait_level;
    std::uint32_t sigil_id;
    std::uint32_t equipped_character;
    std::uint32_t sigil_level;
    std::uint32_t acquisition_count;
    std::uint32_t notification_enum;
};
static_assert(sizeof(SigilEntryRaw) == gbfr::signatures::offset::gem_list::kEntryStride,
              "Sigil entry stride mismatch");

constexpr std::uint32_t kGemListMaxCount      = gbfr::signatures::count::kGemListMax;
constexpr std::uint32_t kGemListEntriesOffset =
    static_cast<std::uint32_t>(gbfr::signatures::offset::gem_list::kEntriesOffset);

void fill_item_name(char* dst, std::size_t dst_size, std::uint32_t hash) {
    if (dst_size == 0) return;
    dst[0] = '\0';
    if (const auto* row = gbfr::find_item_type(hash)) {
        const auto n = std::min<std::size_t>(dst_size - 1, row->name.size());
        std::memcpy(dst, row->name.data(), n);
        dst[n] = '\0';
    }
}

void fill_skill_name(char* dst, std::size_t dst_size, std::uint32_t hash) {
    if (dst_size == 0) return;
    dst[0] = '\0';
    if (const auto* row = gbfr::find_skill(hash)) {
        const auto n = std::min<std::size_t>(dst_size - 1, row->name.size());
        std::memcpy(dst, row->name.data(), n);
        dst[n] = '\0';
    }
}

void fill_character_id(char* dst, std::size_t dst_size, std::uint32_t hash) {
    if (dst_size == 0) return;
    dst[0] = '\0';
    if (const auto row = gbfr::find_character_type(hash)) {
        const auto n = std::min<std::size_t>(dst_size - 1, row->asset_id.size());
        std::memcpy(dst, row->asset_id.data(), n);
        dst[n] = '\0';
    }
}

bool sigil_entry_is_live(const SigilEntryRaw& e) {
    // The empty-key sentinel `hash("")` is the engine's "this
    // slot is empty" marker; non-zero sigil_id != sentinel = real entry.
    return e.sigil_id != 0 && e.sigil_id != gbfr::signatures::sentinel::kEmptyKey;
}

} // namespace

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_sigil_get_count(uint32_t* out_count) {
    if (!out_count) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;

    const gbfr::Address list = s->find_save_list(gbfr::signatures::vft::kGemList);
    if (list == 0) { *out_count = 0; return GBFR_ERR_NOT_FOUND; }

    std::uint32_t live = 0;
    for (std::uint32_t i = 0; i < kGemListMaxCount; ++i) {
        const auto addr = list + kGemListEntriesOffset
                        + static_cast<gbfr::Address>(i) * sizeof(SigilEntryRaw);
        SigilEntryRaw e{};
        if (!s->memory().read(addr, &e, sizeof(e))) continue;
        if (sigil_entry_is_live(e)) ++live;
    }
    *out_count = live;
    return GBFR_OK;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_sigil_get_at(uint32_t index,
                                                          GbfrSigilInfo* out_info) {
    if (!out_info) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    std::memset(out_info, 0, sizeof(*out_info));

    const gbfr::Address list = s->find_save_list(gbfr::signatures::vft::kGemList);
    if (list == 0) return GBFR_ERR_NOT_FOUND;

    std::uint32_t live = 0;
    for (std::uint32_t i = 0; i < kGemListMaxCount; ++i) {
        const auto addr = list + kGemListEntriesOffset
                        + static_cast<gbfr::Address>(i) * sizeof(SigilEntryRaw);
        SigilEntryRaw e{};
        if (!s->memory().read(addr, &e, sizeof(e))) continue;
        if (!sigil_entry_is_live(e)) continue;
        if (live == index) {
            out_info->first_trait_id     = e.first_trait_id;
            out_info->first_trait_level  = e.first_trait_level;
            out_info->second_trait_id    = e.second_trait_id;
            out_info->second_trait_level = e.second_trait_level;
            out_info->sigil_id           = e.sigil_id;
            out_info->equipped_character = e.equipped_character;
            out_info->sigil_level        = e.sigil_level;
            out_info->acquisition_count  = e.acquisition_count;
            out_info->notification_enum  = e.notification_enum;
            fill_skill_name(out_info->first_trait_name,
                            sizeof(out_info->first_trait_name), e.first_trait_id);
            fill_skill_name(out_info->second_trait_name,
                            sizeof(out_info->second_trait_name), e.second_trait_id);
            fill_item_name(out_info->sigil_name,
                           sizeof(out_info->sigil_name), e.sigil_id);
            fill_character_id(out_info->equipped_character_id,
                              sizeof(out_info->equipped_character_id),
                              e.equipped_character);
            return GBFR_OK;
        }
        ++live;
    }
    return GBFR_ERR_OUT_OF_RANGE;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_sigil_set_fields(
    uint32_t    index,
    uint32_t    sigil_level,
    const char* trait1_name,
    uint32_t    trait1_level,
    const char* trait2_name,
    uint32_t    trait2_level) {
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;

    const gbfr::Address list = s->find_save_list(gbfr::signatures::vft::kGemList);
    if (list == 0) return GBFR_ERR_NOT_FOUND;

    // Resolve trait names up front so a bad name aborts before any writes.
    std::uint32_t t1_hash = 0;
    std::uint32_t t2_hash = 0;
    const bool have_t1 = trait1_name && trait1_name[0] != '\0';
    const bool have_t2 = trait2_name && trait2_name[0] != '\0';
    if (have_t1) {
        const auto* row = gbfr::find_skill_by_name(trait1_name);
        if (!row) return GBFR_ERR_NOT_FOUND;
        t1_hash = row->hash;
    }
    if (have_t2) {
        const auto* row = gbfr::find_skill_by_name(trait2_name);
        if (!row) return GBFR_ERR_NOT_FOUND;
        t2_hash = row->hash;
    }

    std::uint32_t live = 0;
    for (std::uint32_t i = 0; i < kGemListMaxCount; ++i) {
        const auto base = list + kGemListEntriesOffset
                        + static_cast<gbfr::Address>(i) * sizeof(SigilEntryRaw);
        SigilEntryRaw e{};
        if (!s->memory().read(base, &e, sizeof(e))) continue;
        if (!sigil_entry_is_live(e)) continue;
        if (live == index) {
            const auto write_u32 = [&](std::ptrdiff_t off, std::uint32_t v) {
                return s->memory().write(base + off, &v, sizeof(v));
            };
            if (!write_u32(offsetof(SigilEntryRaw, sigil_level), sigil_level))
                return GBFR_ERR_PLATFORM;
            if (have_t1) {
                if (!write_u32(offsetof(SigilEntryRaw, first_trait_id), t1_hash))
                    return GBFR_ERR_PLATFORM;
            }
            if (!write_u32(offsetof(SigilEntryRaw, first_trait_level), trait1_level))
                return GBFR_ERR_PLATFORM;
            if (have_t2) {
                if (!write_u32(offsetof(SigilEntryRaw, second_trait_id), t2_hash))
                    return GBFR_ERR_PLATFORM;
            }
            if (!write_u32(offsetof(SigilEntryRaw, second_trait_level), trait2_level))
                return GBFR_ERR_PLATFORM;
            return GBFR_OK;
        }
        ++live;
    }
    return GBFR_ERR_OUT_OF_RANGE;
}

// ---------------------------------------------------------------------------
// 7. Wrightstones (sys::data::ItemPendulumList walking)
// ---------------------------------------------------------------------------

namespace {

// Wrightstone entry layout (stride 0x30, verified live):
//   +0x00 (u32, u32) trait1 (skill hash, level)
//   +0x08 (u32, u32) trait2 (skill hash, level)
//   +0x10 (u32, u32) trait3 (skill hash, level)
//   +0x18  u32      template_hash       (ITEM_25/26/27/28_xxxx asset)
//   +0x1C  u32      acquired_seq
//   +0x20  u32      notification_enum
//   +0x24..+0x2F  padding (zeros)
struct WrightstoneEntryRaw {
    std::uint32_t trait1_id;
    std::uint32_t trait1_level;
    std::uint32_t trait2_id;
    std::uint32_t trait2_level;
    std::uint32_t trait3_id;
    std::uint32_t trait3_level;
    std::uint32_t template_hash;
    std::uint32_t acquired_seq;
    std::uint32_t notification_enum;
    std::uint32_t pad0;
    std::uint32_t pad1;
    std::uint32_t pad2;
};
static_assert(sizeof(WrightstoneEntryRaw) == gbfr::signatures::offset::item_pendulum_list::kEntryStride,
              "Wrightstone stride mismatch");

constexpr std::uint32_t kPendulumMaxCount      = gbfr::signatures::count::kItemPendulumListMax;
constexpr std::uint32_t kPendulumEntriesOffset =
    static_cast<std::uint32_t>(gbfr::signatures::offset::item_pendulum_list::kEntriesOffset);

bool wrightstone_entry_is_live(const WrightstoneEntryRaw& e) {
    // Live entries have a non-empty template_hash (not zero, not hash("")).
    return e.template_hash != 0 && e.template_hash != gbfr::signatures::sentinel::kEmptyKey;
}

} // namespace

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_wrightstone_get_count(uint32_t* out_count) {
    if (!out_count) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;

    const gbfr::Address list = s->find_save_list(gbfr::signatures::vft::kItemPendulumList);
    if (list == 0) { *out_count = 0; return GBFR_ERR_NOT_FOUND; }

    std::uint32_t live = 0;
    for (std::uint32_t i = 0; i < kPendulumMaxCount; ++i) {
        const auto addr = list + kPendulumEntriesOffset
                        + static_cast<gbfr::Address>(i) * sizeof(WrightstoneEntryRaw);
        WrightstoneEntryRaw e{};
        if (!s->memory().read(addr, &e, sizeof(e))) continue;
        if (wrightstone_entry_is_live(e)) ++live;
    }
    *out_count = live;
    return GBFR_OK;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_wrightstone_get_at(uint32_t index,
                                                                GbfrWrightstoneInfo* out_info) {
    if (!out_info) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    std::memset(out_info, 0, sizeof(*out_info));

    const gbfr::Address list = s->find_save_list(gbfr::signatures::vft::kItemPendulumList);
    if (list == 0) return GBFR_ERR_NOT_FOUND;

    std::uint32_t live = 0;
    for (std::uint32_t i = 0; i < kPendulumMaxCount; ++i) {
        const auto addr = list + kPendulumEntriesOffset
                        + static_cast<gbfr::Address>(i) * sizeof(WrightstoneEntryRaw);
        WrightstoneEntryRaw e{};
        if (!s->memory().read(addr, &e, sizeof(e))) continue;
        if (!wrightstone_entry_is_live(e)) continue;
        if (live == index) {
            out_info->trait1_id          = e.trait1_id;
            out_info->trait1_level       = e.trait1_level;
            out_info->trait2_id          = e.trait2_id;
            out_info->trait2_level       = e.trait2_level;
            out_info->trait3_id          = e.trait3_id;
            out_info->trait3_level       = e.trait3_level;
            out_info->template_hash      = e.template_hash;
            out_info->acquired_seq       = e.acquired_seq;
            out_info->notification_enum  = e.notification_enum;
            fill_skill_name(out_info->trait1_name, sizeof(out_info->trait1_name), e.trait1_id);
            fill_skill_name(out_info->trait2_name, sizeof(out_info->trait2_name), e.trait2_id);
            fill_skill_name(out_info->trait3_name, sizeof(out_info->trait3_name), e.trait3_id);
            if (const auto* row = gbfr::find_item_type(e.template_hash)) {
                const auto n_id = std::min<std::size_t>(63, row->asset_id.size());
                const auto n_nm = std::min<std::size_t>(63, row->name.size());
                std::memcpy(out_info->template_asset_id, row->asset_id.data(), n_id);
                std::memcpy(out_info->template_name,     row->name.data(),     n_nm);
            }
            return GBFR_OK;
        }
        ++live;
    }
    return GBFR_ERR_OUT_OF_RANGE;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_wrightstone_set_fields(
    uint32_t    index,
    const char* trait1_name, uint32_t trait1_level,
    const char* trait2_name, uint32_t trait2_level,
    const char* trait3_name, uint32_t trait3_level) {
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;

    const gbfr::Address list = s->find_save_list(gbfr::signatures::vft::kItemPendulumList);
    if (list == 0) return GBFR_ERR_NOT_FOUND;

    // Resolve names up front.
    std::uint32_t hashes[3] = {0, 0, 0};
    bool have[3] = {false, false, false};
    const char* names[3]    = {trait1_name, trait2_name, trait3_name};
    for (int i = 0; i < 3; ++i) {
        if (names[i] && names[i][0] != '\0') {
            const auto* row = gbfr::find_skill_by_name(names[i]);
            if (!row) return GBFR_ERR_NOT_FOUND;
            hashes[i] = row->hash;
            have[i]   = true;
        }
    }
    const std::uint32_t levels[3] = {trait1_level, trait2_level, trait3_level};

    std::uint32_t live = 0;
    for (std::uint32_t i = 0; i < kPendulumMaxCount; ++i) {
        const auto base = list + kPendulumEntriesOffset
                        + static_cast<gbfr::Address>(i) * sizeof(WrightstoneEntryRaw);
        WrightstoneEntryRaw e{};
        if (!s->memory().read(base, &e, sizeof(e))) continue;
        if (!wrightstone_entry_is_live(e)) continue;
        if (live == index) {
            const auto write_u32 = [&](std::ptrdiff_t off, std::uint32_t v) {
                return s->memory().write(base + off, &v, sizeof(v));
            };
            const std::ptrdiff_t id_offs[3]    = {
                offsetof(WrightstoneEntryRaw, trait1_id),
                offsetof(WrightstoneEntryRaw, trait2_id),
                offsetof(WrightstoneEntryRaw, trait3_id),
            };
            const std::ptrdiff_t level_offs[3] = {
                offsetof(WrightstoneEntryRaw, trait1_level),
                offsetof(WrightstoneEntryRaw, trait2_level),
                offsetof(WrightstoneEntryRaw, trait3_level),
            };
            for (int k = 0; k < 3; ++k) {
                if (have[k] && !write_u32(id_offs[k], hashes[k])) {
                    return GBFR_ERR_PLATFORM;
                }
                if (!write_u32(level_offs[k], levels[k])) {
                    return GBFR_ERR_PLATFORM;
                }
            }
            return GBFR_OK;
        }
        ++live;
    }
    return GBFR_ERR_OUT_OF_RANGE;
}

// ---------------------------------------------------------------------------
// 3.5 Currencies
// ---------------------------------------------------------------------------

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_currency_get_info(GbfrCurrencyInfo* out_info) {
    if (!out_info) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    const gbfr::Address base = s->find_user_save_block();
    if (base == 0) return GBFR_ERR_NOT_FOUND;
    std::uint32_t rupies = 0;
    std::uint32_t mastery = 0;
    namespace ofs = gbfr::signatures::offset::user_save_block;
    if (!s->memory().read(base + ofs::kRupiesU32, &rupies, sizeof(rupies)) ||
        !s->memory().read(base + ofs::kMasteryPointsU32, &mastery, sizeof(mastery))) {
        return GBFR_ERR_PLATFORM;
    }
    out_info->wallet_address = static_cast<uint64_t>(base);
    out_info->rupies         = rupies;
    out_info->mastery_points = mastery;
    return GBFR_OK;
}

static GbfrStatus currency_set_field(std::ptrdiff_t field_off, uint32_t value) {
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    const gbfr::Address base = s->find_user_save_block();
    if (base == 0) return GBFR_ERR_NOT_FOUND;
    if (!s->memory().write(base + field_off, &value, sizeof(value))) {
        return GBFR_ERR_PLATFORM;
    }
    return GBFR_OK;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_currency_set_rupies(uint32_t value) {
    return currency_set_field(gbfr::signatures::offset::user_save_block::kRupiesU32, value);
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_currency_set_mastery_points(uint32_t value) {
    return currency_set_field(gbfr::signatures::offset::user_save_block::kMasteryPointsU32, value);
}

// ---------------------------------------------------------------------------
// 4. Debug primitives
// ---------------------------------------------------------------------------

} // extern "C"

extern "C" {

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_module_info(GbfrModuleInfo* out) {
    if (!out) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    out->base = static_cast<uint64_t>(gbfr::core::debug::module_base(*s));
    out->size = static_cast<uint64_t>(gbfr::core::debug::module_size(*s));
    return GBFR_OK;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_rva_to_abs(uint64_t rva, uint64_t* out_abs) {
    if (!out_abs) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    *out_abs = static_cast<uint64_t>(
        gbfr::core::debug::rva_to_abs(*s, static_cast<gbfr::Address>(rva)));
    return GBFR_OK;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_u8 (uint64_t a, uint8_t*  o) { return dbg_read_typed(a, o); }
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_u32(uint64_t a, uint32_t* o) { return dbg_read_typed(a, o); }
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_u64(uint64_t a, uint64_t* o) { return dbg_read_typed(a, o); }
GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_f32(uint64_t a, float*    o) { return dbg_read_typed(a, o); }

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_bytes(uint64_t addr, void* out, uint32_t n) {
    if (!out && n != 0) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    return gbfr::core::debug::read(*s, static_cast<gbfr::Address>(addr), out, n)
        ? GBFR_OK : GBFR_ERR_PLATFORM;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_read_cstr(uint64_t addr, char* out, uint32_t cap) {
    if (!out || cap == 0) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    auto str = gbfr::core::debug::read_cstr(*s, static_cast<gbfr::Address>(addr), cap - 1);
    const std::size_t copy = std::min<std::size_t>(str.size(), cap - 1);
    std::memcpy(out, str.data(), copy);
    out[copy] = '\0';
    return GBFR_OK;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_qword(uint64_t value, uint64_t* out_addr) {
    if (!out_addr) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    const auto a = gbfr::core::debug::scan_qword(*s, value);
    *out_addr = static_cast<uint64_t>(a);
    return a ? GBFR_OK : GBFR_ERR_NOT_FOUND;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_pattern(const char* ida_pattern, uint64_t* out_addr) {
    if (!ida_pattern || !out_addr) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    const auto a = gbfr::core::debug::scan_pattern(*s, ida_pattern);
    *out_addr = static_cast<uint64_t>(a);
    return a ? GBFR_OK : GBFR_ERR_NOT_FOUND;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_qword_all(
    uint64_t value, uint64_t* out_addrs, uint32_t cap, uint32_t* out_count) {
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    return copy_addr_vec(
        gbfr::core::debug::scan_qword_all(*s, value, cap ? cap : 64),
        out_addrs, cap, out_count);
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_qword_in_process(
    uint64_t value, uint64_t* out_addrs, uint32_t cap, uint32_t* out_count) {
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    return copy_addr_vec(
        gbfr::core::debug::scan_qword_in_process(*s, value, cap ? cap : 64),
        out_addrs, cap, out_count);
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_dword_in_process(
    uint32_t value, uint64_t* out_addrs, uint32_t cap, uint32_t* out_count) {
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    return copy_addr_vec(
        gbfr::core::debug::scan_dword_in_process(*s, value, cap ? cap : 64),
        out_addrs, cap, out_count);
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_scan_pattern_all(
    const char* ida_pattern, uint64_t* out_addrs, uint32_t cap, uint32_t* out_count) {
    if (!ida_pattern) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    return copy_addr_vec(
        gbfr::core::debug::scan_pattern_all(*s, ida_pattern, cap ? cap : 32),
        out_addrs, cap, out_count);
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_find_lea_refs_to(
    uint64_t target, uint64_t* out_addrs, uint32_t cap, uint32_t* out_count) {
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    return copy_addr_vec(
        gbfr::core::debug::find_lea_refs_to(*s, static_cast<gbfr::Address>(target),
                                            cap ? cap : 32),
        out_addrs, cap, out_count);
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_save_aggregate(uint64_t* out_addr) {
    if (!out_addr) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    const auto a = s->save_aggregate_address();
    *out_addr = static_cast<uint64_t>(a);
    return a ? GBFR_OK : GBFR_ERR_NOT_FOUND;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_player_data_offset(uint32_t* out_off) {
    if (!out_off) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    const auto off = s->player_data_offset();
    *out_off = off;
    return off ? GBFR_OK : GBFR_ERR_NOT_FOUND;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_app_main_loop(uint64_t* out_addr) {
    if (!out_addr) return GBFR_ERR_INVALID_ARG;
    auto* s = borrow_session_or_null();
    if (!s) return GBFR_ERR_NOT_INIT;
    *out_addr = static_cast<uint64_t>(
        s->rva(gbfr::signatures::instance::kAppMainLoop));
    return GBFR_OK;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_debug_hash32(const char* str, uint32_t* out_hash) {
    if (!str || !out_hash) return GBFR_ERR_INVALID_ARG;
    *out_hash = gbfr::StringHash32(std::string_view{str}).value();
    return GBFR_OK;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_character_type_lookup(
    uint32_t hash, char* out_asset_id, char* out_name) {
    const auto row = gbfr::find_character_type(hash);
    if (!row) {
        if (out_asset_id) out_asset_id[0] = '\0';
        if (out_name)     out_name[0]     = '\0';
        return GBFR_ERR_NOT_FOUND;
    }
    if (out_asset_id) {
        const auto n = std::min<std::size_t>(63, row->asset_id.size());
        std::memcpy(out_asset_id, row->asset_id.data(), n);
        out_asset_id[n] = '\0';
    }
    if (out_name) {
        const auto n = std::min<std::size_t>(63, row->name.size());
        std::memcpy(out_name, row->name.data(), n);
        out_name[n] = '\0';
    }
    return GBFR_OK;
}

} // extern "C"
