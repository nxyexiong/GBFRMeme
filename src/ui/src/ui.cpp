// SPDX-License-Identifier: MIT
// src/ui.cpp — ImGui draw code for the GBFR UI.
//
// One window per `gbfr_core` C-API section. Section bodies are empty for
// now and will be filled in as the corresponding API surface stabilises.

#include "gbfr/ui/ui.hpp"

#include "gbfr/core/c_api.h"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

namespace gbfr::ui {

namespace {

bool name_contains_ci(const char* name, const char* needle) {
    if (!needle || needle[0] == '\0') return true;
    if (!name) return false;
    const std::size_t nlen = std::strlen(name);
    const std::size_t klen = std::strlen(needle);
    if (klen > nlen) return false;
    for (std::size_t i = 0; i + klen <= nlen; ++i) {
        bool match = true;
        for (std::size_t j = 0; j < klen; ++j) {
            const auto a = static_cast<unsigned char>(name[i + j]);
            const auto b = static_cast<unsigned char>(needle[j]);
            if (std::tolower(a) != std::tolower(b)) { match = false; break; }
        }
        if (match) return true;
    }
    return false;
}

// Filter input may carry multiple comma-separated tokens; whitespace around
// each token is trimmed. Empty filter / empty token-set => match everything.
// For a non-empty filter the row passes only if EVERY token matches at
// least one of the provided candidate fields (case-insensitive substring).
bool any_field_matches_filter(const char* filter,
                              std::initializer_list<const char*> fields) {
    if (!filter || filter[0] == '\0') return true;

    auto token_matches_any_field = [&](const char* tok_begin, std::size_t tok_len) {
        if (tok_len == 0) return true;
        char buf[64];
        if (tok_len >= sizeof(buf)) tok_len = sizeof(buf) - 1;
        std::memcpy(buf, tok_begin, tok_len);
        buf[tok_len] = '\0';
        for (const char* f : fields) {
            if (name_contains_ci(f, buf)) return true;
        }
        return false;
    };

    const char* p = filter;
    bool any_non_empty_token = false;
    while (*p) {
        while (*p == ' ' || *p == '\t') ++p;
        const char* tok = p;
        while (*p && *p != ',') ++p;
        const char* tok_end = p;
        while (tok_end > tok && (tok_end[-1] == ' ' || tok_end[-1] == '\t')) --tok_end;
        const std::size_t tok_len = (std::size_t)(tok_end - tok);
        if (tok_len > 0) {
            any_non_empty_token = true;
            if (!token_matches_any_field(tok, tok_len)) return false;
        }
        if (*p == ',') ++p;
    }
    (void)any_non_empty_token;
    return true;
}

// Strip a trailing " (NN_NNNN)" tag and any case-insensitive occurrence of
// " Wrightstone" / "Wrightstone " to keep wrightstone template names short
// in the UI. Writes into `out` (must be at least 1 byte).
void shorten_wrightstone_name(const char* src, char* out, std::size_t cap) {
    if (cap == 0) return;
    out[0] = '\0';
    if (!src) return;

    // 1) Copy `src` minus a trailing " (xx_yyyy)" suffix.
    std::size_t n = std::strlen(src);
    if (n >= 9 && src[n - 1] == ')') {
        std::size_t open = n;
        for (std::size_t i = n; i-- > 0; ) {
            if (src[i] == '(') { open = i; break; }
        }
        if (open + 1 < n && open >= 1 && src[open - 1] == ' ') {
            n = open - 1;
        }
    }
    if (n >= cap) n = cap - 1;
    std::memcpy(out, src, n);
    out[n] = '\0';

    // 2) Remove case-insensitive "Wrightstone" (and one adjacent space).
    static constexpr char kKey[]   = "Wrightstone";
    constexpr std::size_t kKeyLen  = sizeof(kKey) - 1;
    for (std::size_t i = 0; i + kKeyLen <= n; ) {
        bool match = true;
        for (std::size_t j = 0; j < kKeyLen; ++j) {
            const auto a = static_cast<unsigned char>(out[i + j]);
            const auto b = static_cast<unsigned char>(kKey[j]);
            if (std::tolower(a) != std::tolower(b)) { match = false; break; }
        }
        if (!match) { ++i; continue; }
        std::size_t start = i;
        std::size_t end   = i + kKeyLen;
        if (end < n && out[end] == ' ')           ++end;
        else if (start > 0 && out[start - 1] == ' ') --start;
        std::memmove(out + start, out + end, n - end + 1);
        n -= (end - start);
    }
}

void section_wallet() {
    if (!ImGui::CollapsingHeader("Wallet", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    static int  s_rupies  = 0;
    static int  s_mastery = 0;
    static char s_status[128] = "";

    auto report = [&](const char* op, GbfrStatus st) {
        if (st == GBFR_OK) {
            std::snprintf(s_status, sizeof(s_status), "%s: OK", op);
        } else {
            std::snprintf(s_status, sizeof(s_status), "%s: status=%d", op, (int)st);
        }
    };

    // Row 1 — Rupies
    ImGui::PushID("rupies");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("rupies:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputInt("##val", &s_rupies, 0);
    if (s_rupies < 0) s_rupies = 0;
    ImGui::SameLine();
    if (ImGui::Button("Get")) {
        GbfrCurrencyInfo info{};
        GbfrStatus st = gbfr_currency_get_info(&info);
        if (st == GBFR_OK) s_rupies = (int)info.rupies;
        report("get rupies", st);
    }
    ImGui::SameLine();
    if (ImGui::Button("Set")) {
        report("set rupies", gbfr_currency_set_rupies((uint32_t)s_rupies));
    }
    ImGui::PopID();

    // Row 2 — Mastery points
    ImGui::PushID("mastery");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("mastery points:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputInt("##val", &s_mastery, 0);
    if (s_mastery < 0) s_mastery = 0;
    ImGui::SameLine();
    if (ImGui::Button("Get")) {
        GbfrCurrencyInfo info{};
        GbfrStatus st = gbfr_currency_get_info(&info);
        if (st == GBFR_OK) s_mastery = (int)info.mastery_points;
        report("get mastery", st);
    }
    ImGui::SameLine();
    if (ImGui::Button("Set")) {
        report("set mastery", gbfr_currency_set_mastery_points((uint32_t)s_mastery));
    }
    ImGui::PopID();

    if (s_status[0]) {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", s_status);
    }
}

void section_items() {
    if (!ImGui::CollapsingHeader("Items", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    // Snapshot of the live ItemList. Each entry mirrors `GbfrItemInfo` plus
    // an edit buffer for `count`. Indexed by `live_index` (same indexing the
    // C API uses), which is stored so writes go to the correct slot even
    // when filtered.
    struct Row {
        std::uint32_t live_index;
        GbfrItemInfo  info;
        int           edit_count;
    };
    static std::vector<Row> s_rows;
    static char             s_filter[64]   = "";
    static char             s_status[128]  = "";

    auto report = [&](const char* op, GbfrStatus st) {
        if (st == GBFR_OK) {
            std::snprintf(s_status, sizeof(s_status), "%s: OK", op);
        } else {
            std::snprintf(s_status, sizeof(s_status), "%s: status=%d", op, (int)st);
        }
    };

    auto refresh_all = [&]() {
        s_rows.clear();
        std::uint32_t count = 0;
        GbfrStatus st = gbfr_item_get_count(&count);
        if (st != GBFR_OK) { report("refresh", st); return; }
        s_rows.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            Row r{};
            r.live_index = i;
            if (gbfr_item_get_at(i, &r.info) == GBFR_OK) {
                r.edit_count = (int)r.info.count;
                s_rows.push_back(r);
            }
        }
        std::snprintf(s_status, sizeof(s_status), "refresh: %zu item(s)", s_rows.size());
    };

    // Top control row
    if (ImGui::Button("Refresh##items")) {
        refresh_all();
    }
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("filter:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(240.0f);
    ImGui::InputText("##filter", s_filter, sizeof(s_filter));

    if (s_status[0]) {
        ImGui::SameLine();
        ImGui::TextDisabled("  %s", s_status);
    }

    ImGui::Spacing();
    ImGui::Separator();

    for (auto& r : s_rows) {
        if (!any_field_matches_filter(s_filter, { r.info.display_name })) continue;
        ImGui::PushID((int)r.live_index);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("id: %-14s", r.info.asset_id[0] ? r.info.asset_id : "<unknown>");
        ImGui::SameLine();
        ImGui::Text("name: %-32s", r.info.display_name[0] ? r.info.display_name : "<unknown>");
        ImGui::SameLine();
        ImGui::TextUnformatted("count:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputInt("##count", &r.edit_count, 0);
        if (r.edit_count < 0) r.edit_count = 0;
        ImGui::SameLine();
        if (ImGui::Button("Get")) {
            GbfrItemInfo fresh{};
            GbfrStatus st = gbfr_item_get_at(r.live_index, &fresh);
            if (st == GBFR_OK) {
                r.info       = fresh;
                r.edit_count = (int)fresh.count;
            }
            report("get item", st);
        }
        ImGui::SameLine();
        if (ImGui::Button("Set")) {
            report("set item",
                   gbfr_item_set_count(r.live_index, (std::uint32_t)r.edit_count));
        }
        ImGui::PopID();
    }
}

void section_sigils() {
    if (!ImGui::CollapsingHeader("Sigils", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    struct Row {
        std::uint32_t  live_index;
        GbfrSigilInfo  info;
        int            edit_level;
        int            edit_t1_level;
        int            edit_t2_level;
        char           edit_t1_name[64];
        char           edit_t2_name[64];
    };
    static std::vector<Row> s_rows;
    static char             s_filter[64]  = "";
    static char             s_status[128] = "";

    auto report = [&](const char* op, GbfrStatus st) {
        if (st == GBFR_OK) {
            std::snprintf(s_status, sizeof(s_status), "%s: OK", op);
        } else {
            std::snprintf(s_status, sizeof(s_status), "%s: status=%d", op, (int)st);
        }
    };

    auto refresh_all = [&]() {
        s_rows.clear();
        std::uint32_t count = 0;
        GbfrStatus st = gbfr_sigil_get_count(&count);
        if (st != GBFR_OK) { report("refresh", st); return; }
        s_rows.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            Row r{};
            r.live_index = i;
            if (gbfr_sigil_get_at(i, &r.info) == GBFR_OK) {
                r.edit_level    = (int)r.info.sigil_level;
                r.edit_t1_level = (int)r.info.first_trait_level;
                r.edit_t2_level = (int)r.info.second_trait_level;
                std::snprintf(r.edit_t1_name, sizeof(r.edit_t1_name), "%s",
                              r.info.first_trait_name);
                std::snprintf(r.edit_t2_name, sizeof(r.edit_t2_name), "%s",
                              r.info.second_trait_name);
                s_rows.push_back(r);
            }
        }
        std::snprintf(s_status, sizeof(s_status), "refresh: %zu sigil(s)", s_rows.size());
    };

    if (ImGui::Button("Refresh##sigils")) {
        refresh_all();
    }
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("filter:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(240.0f);
    ImGui::InputText("##sigil_filter", s_filter, sizeof(s_filter));
    if (s_status[0]) {
        ImGui::SameLine();
        ImGui::TextDisabled("  %s", s_status);
    }

    ImGui::Spacing();
    ImGui::Separator();

    for (auto& r : s_rows) {
        // Filter against sigil name + both trait names.
        if (!any_field_matches_filter(s_filter,
                { r.info.sigil_name,
                  r.info.first_trait_name,
                  r.info.second_trait_name })) {
            continue;
        }

        ImGui::PushID((int)r.live_index);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("[%4u]", r.live_index);
            ImGui::SameLine();
            ImGui::Text("%-28s", r.info.sigil_name[0] ? r.info.sigil_name : "<unknown>");
            ImGui::SameLine();
            ImGui::TextUnformatted("level:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            ImGui::InputInt("##lvl", &r.edit_level, 0);
            if (r.edit_level < 0) r.edit_level = 0;
            ImGui::SameLine();
            ImGui::SetNextItemWidth(180.0f);
            ImGui::InputText("##t1", r.edit_t1_name, sizeof(r.edit_t1_name));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(40.0f);
            ImGui::InputInt("##t1lvl", &r.edit_t1_level, 0);
            if (r.edit_t1_level < 0) r.edit_t1_level = 0;
            ImGui::SameLine();
            ImGui::SetNextItemWidth(180.0f);
            ImGui::InputText("##t2", r.edit_t2_name, sizeof(r.edit_t2_name));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(40.0f);
            ImGui::InputInt("##t2lvl", &r.edit_t2_level, 0);
            if (r.edit_t2_level < 0) r.edit_t2_level = 0;
            ImGui::SameLine();
            if (ImGui::Button("Get")) {
                GbfrSigilInfo fresh{};
                GbfrStatus st = gbfr_sigil_get_at(r.live_index, &fresh);
                if (st == GBFR_OK) {
                    r.info          = fresh;
                    r.edit_level    = (int)fresh.sigil_level;
                    r.edit_t1_level = (int)fresh.first_trait_level;
                    r.edit_t2_level = (int)fresh.second_trait_level;
                    std::snprintf(r.edit_t1_name, sizeof(r.edit_t1_name), "%s",
                                  fresh.first_trait_name);
                    std::snprintf(r.edit_t2_name, sizeof(r.edit_t2_name), "%s",
                                  fresh.second_trait_name);
                }
                report("get sigil", st);
            }
            ImGui::SameLine();
            if (ImGui::Button("Set")) {
                report("set sigil",
                       gbfr_sigil_set_fields(
                           r.live_index,
                           (std::uint32_t)r.edit_level,
                           r.edit_t1_name,
                           (std::uint32_t)r.edit_t1_level,
                           r.edit_t2_name,
                           (std::uint32_t)r.edit_t2_level));
            }
            ImGui::PopID();
    }
}

void section_wrightstones() {
    if (!ImGui::CollapsingHeader("Wrightstones", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    struct Row {
        std::uint32_t       live_index;
        GbfrWrightstoneInfo info;
        int                 edit_t1_level;
        int                 edit_t2_level;
        int                 edit_t3_level;
        char                edit_t1_name[64];
        char                edit_t2_name[64];
        char                edit_t3_name[64];
    };
    static std::vector<Row> s_rows;
    static char             s_filter[64]  = "";
    static char             s_status[128] = "";

    auto report = [&](const char* op, GbfrStatus st) {
        if (st == GBFR_OK) {
            std::snprintf(s_status, sizeof(s_status), "%s: OK", op);
        } else {
            std::snprintf(s_status, sizeof(s_status), "%s: status=%d", op, (int)st);
        }
    };

    auto load_row = [](Row& r, const GbfrWrightstoneInfo& info) {
        r.info          = info;
        r.edit_t1_level = (int)info.trait1_level;
        r.edit_t2_level = (int)info.trait2_level;
        r.edit_t3_level = (int)info.trait3_level;
        std::snprintf(r.edit_t1_name, sizeof(r.edit_t1_name), "%s", info.trait1_name);
        std::snprintf(r.edit_t2_name, sizeof(r.edit_t2_name), "%s", info.trait2_name);
        std::snprintf(r.edit_t3_name, sizeof(r.edit_t3_name), "%s", info.trait3_name);
    };

    auto refresh_all = [&]() {
        s_rows.clear();
        std::uint32_t count = 0;
        GbfrStatus st = gbfr_wrightstone_get_count(&count);
        if (st != GBFR_OK) { report("refresh", st); return; }
        s_rows.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            Row r{};
            r.live_index = i;
            GbfrWrightstoneInfo info{};
            if (gbfr_wrightstone_get_at(i, &info) == GBFR_OK) {
                load_row(r, info);
                s_rows.push_back(r);
            }
        }
        std::snprintf(s_status, sizeof(s_status), "refresh: %zu wrightstone(s)", s_rows.size());
    };

    if (ImGui::Button("Refresh##wrightstones")) {
        refresh_all();
    }
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("filter:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(240.0f);
    ImGui::InputText("##wrightstone_filter", s_filter, sizeof(s_filter));
    if (s_status[0]) {
        ImGui::SameLine();
        ImGui::TextDisabled("  %s", s_status);
    }

    ImGui::Spacing();
    ImGui::Separator();

    for (auto& r : s_rows) {
        if (!any_field_matches_filter(s_filter,
                { r.info.template_name,
                  r.info.trait1_name,
                  r.info.trait2_name,
                  r.info.trait3_name })) {
            continue;
        }

        ImGui::PushID((int)r.live_index);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("[%4u]", r.live_index);
        ImGui::SameLine();
        {
            char short_name[64];
            shorten_wrightstone_name(r.info.template_name, short_name, sizeof(short_name));
            ImGui::Text("%-16s", short_name[0] ? short_name : "<unknown>");
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputText("##t1", r.edit_t1_name, sizeof(r.edit_t1_name));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40.0f);
        ImGui::InputInt("##t1lvl", &r.edit_t1_level, 0);
        if (r.edit_t1_level < 0) r.edit_t1_level = 0;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputText("##t2", r.edit_t2_name, sizeof(r.edit_t2_name));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40.0f);
        ImGui::InputInt("##t2lvl", &r.edit_t2_level, 0);
        if (r.edit_t2_level < 0) r.edit_t2_level = 0;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputText("##t3", r.edit_t3_name, sizeof(r.edit_t3_name));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40.0f);
        ImGui::InputInt("##t3lvl", &r.edit_t3_level, 0);
        if (r.edit_t3_level < 0) r.edit_t3_level = 0;
        ImGui::SameLine();
        if (ImGui::Button("Get")) {
            GbfrWrightstoneInfo fresh{};
            GbfrStatus st = gbfr_wrightstone_get_at(r.live_index, &fresh);
            if (st == GBFR_OK) load_row(r, fresh);
            report("get wrightstone", st);
        }
        ImGui::SameLine();
        if (ImGui::Button("Set")) {
            report("set wrightstone",
                   gbfr_wrightstone_set_fields(
                       r.live_index,
                       r.edit_t1_name, (std::uint32_t)r.edit_t1_level,
                       r.edit_t2_name, (std::uint32_t)r.edit_t2_level,
                       r.edit_t3_name, (std::uint32_t)r.edit_t3_level));
        }
        ImGui::PopID();
    }
}

void section_summons() {
    if (!ImGui::CollapsingHeader("Summons", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    struct Row {
        std::uint32_t  live_index;
        GbfrSummonInfo info;
        int            edit_trait_level;
        int            edit_stat_value;
        std::uint32_t  edit_stat_type_id;
        bool           edit_stat_is_percent;
        char           edit_trait_name[64];
    };
    static std::vector<Row> s_rows;
    static std::vector<GbfrSummonStatTypeInfo> s_stat_types;
    static char             s_filter[64]  = "";
    static char             s_status[128] = "";

    auto report = [&](const char* op, GbfrStatus st) {
        if (st == GBFR_OK) {
            std::snprintf(s_status, sizeof(s_status), "%s: OK", op);
        } else {
            std::snprintf(s_status, sizeof(s_status), "%s: status=%d", op, (int)st);
        }
    };

    auto load_row = [](Row& row, const GbfrSummonInfo& info) {
        row.info             = info;
        row.edit_trait_level = (int)info.trait_level;
        row.edit_stat_value  = (int)info.stat_value;
        row.edit_stat_type_id = info.stat_type_id;
        row.edit_stat_is_percent = info.stat_is_percent != 0;
        std::snprintf(row.edit_trait_name, sizeof(row.edit_trait_name), "%s",
                      info.trait_name);
    };

    auto refresh_all = [&]() {
        s_rows.clear();
        s_stat_types.clear();

        std::uint32_t stat_type_count = 0;
        GbfrStatus st = gbfr_summon_stat_type_get_count(&stat_type_count);
        if (st != GBFR_OK) { report("load stat types", st); return; }
        s_stat_types.reserve(stat_type_count);
        for (std::uint32_t i = 0; i < stat_type_count; ++i) {
            GbfrSummonStatTypeInfo info{};
            if (gbfr_summon_stat_type_get_at(i, &info) == GBFR_OK) {
                s_stat_types.push_back(info);
            }
        }

        std::uint32_t count = 0;
        st = gbfr_summon_get_count(&count);
        if (st != GBFR_OK) { report("refresh", st); return; }
        s_rows.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            Row row{};
            row.live_index = i;
            GbfrSummonInfo info{};
            st = gbfr_summon_get_at(i, &info);
            if (st != GBFR_OK) {
                s_rows.clear();
                report("refresh", st);
                return;
            }
            load_row(row, info);
            s_rows.push_back(row);
        }
        std::snprintf(s_status, sizeof(s_status),
                      "refresh: %zu summon(s)", s_rows.size());
    };

    if (ImGui::Button("Refresh##summons")) {
        refresh_all();
    }
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("filter:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(240.0f);
    ImGui::InputText("##summon_filter", s_filter, sizeof(s_filter));
    if (s_status[0]) {
        ImGui::SameLine();
        ImGui::TextDisabled("  %s", s_status);
    }

    ImGui::Spacing();
    ImGui::Separator();

    for (auto& row : s_rows) {
        if (!any_field_matches_filter(
                s_filter,
                {row.info.summon_name,
                 row.info.trait_name,
                 row.info.stat_type_name})) {
            continue;
        }

        ImGui::PushID((int)row.live_index);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("[%3u] %-24s",
                    row.live_index,
                    row.info.summon_name[0]
                        ? row.info.summon_name : "<unknown>");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        ImGui::InputText("##trait", row.edit_trait_name,
                         sizeof(row.edit_trait_name));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(45.0f);
        ImGui::InputInt("##trait_level", &row.edit_trait_level, 0);
        if (row.edit_trait_level < 0) row.edit_trait_level = 0;
        ImGui::SameLine();
        const GbfrSummonStatTypeInfo* selected_stat = nullptr;
        for (const auto& stat_type : s_stat_types) {
            if (stat_type.stat_type_id == row.edit_stat_type_id) {
                selected_stat = &stat_type;
                break;
            }
        }
        char stat_preview[128];
        if (selected_stat) {
            std::snprintf(stat_preview, sizeof(stat_preview), "%s [%08X]",
                          selected_stat->name, selected_stat->stat_type_id);
        } else {
            std::snprintf(stat_preview, sizeof(stat_preview), "0x%08X",
                          row.edit_stat_type_id);
        }
        ImGui::SetNextItemWidth(205.0f);
        if (ImGui::BeginCombo("##stat_type", stat_preview)) {
            for (const auto& stat_type : s_stat_types) {
                char option_label[128];
                std::snprintf(option_label, sizeof(option_label), "%s [%08X]",
                              stat_type.name, stat_type.stat_type_id);
                const bool selected =
                    stat_type.stat_type_id == row.edit_stat_type_id;
                if (ImGui::Selectable(option_label, selected)) {
                    row.edit_stat_type_id = stat_type.stat_type_id;
                    row.edit_stat_is_percent = stat_type.is_percent != 0;
                    const auto level = std::min<std::uint32_t>(
                        row.info.stat_level, GBFR_SUMMON_STAT_VALUE_COUNT - 1);
                    row.edit_stat_value = (int)stat_type.values[level];
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(65.0f);
        ImGui::InputInt("##stat_value", &row.edit_stat_value, 0);
        if (row.edit_stat_value < 0) row.edit_stat_value = 0;
        if (row.edit_stat_is_percent) {
            ImGui::SameLine();
            ImGui::TextUnformatted("%");
        }
        ImGui::SameLine();
        if (ImGui::Button("Get")) {
            GbfrSummonInfo fresh{};
            GbfrStatus st = gbfr_summon_get_by_storage_index(
                row.info.storage_index, &fresh);
            if (st == GBFR_OK &&
                (fresh.summon_id != row.info.summon_id ||
                 fresh.unknown_04 != row.info.unknown_04)) {
                st = GBFR_ERR_NOT_FOUND;
            }
            if (st == GBFR_OK) load_row(row, fresh);
            report("get summon", st);
        }
        ImGui::SameLine();
        if (ImGui::Button("Set")) {
            GbfrStatus st = gbfr_summon_set_fields(
                row.info.storage_index,
                row.info.summon_id,
                row.info.unknown_04,
                row.edit_trait_name,
                (std::uint32_t)row.edit_trait_level,
                row.edit_stat_type_id,
                (std::uint32_t)row.edit_stat_value);
            if (st == GBFR_OK) {
                GbfrSummonInfo fresh{};
                st = gbfr_summon_get_by_storage_index(
                    row.info.storage_index, &fresh);
                if (st == GBFR_OK &&
                    (fresh.summon_id != row.info.summon_id ||
                     fresh.unknown_04 != row.info.unknown_04)) {
                    st = GBFR_ERR_NOT_FOUND;
                }
                if (st == GBFR_OK) load_row(row, fresh);
            }
            report("set summon", st);
        }
        ImGui::PopID();
    }
}

} // namespace

void draw() {
    // Borderless ImGui window that exactly fills the host's client area.
    // The Win32 host is non-resizable, so this stays in sync.
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("GBFRMeme", nullptr, flags)) {
        section_wallet();
        section_items();
        section_sigils();
        section_wrightstones();
        section_summons();
    }
    ImGui::End();
}

} // namespace gbfr::ui
