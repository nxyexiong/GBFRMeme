// SPDX-License-Identifier: MIT
#include "cli_debug.hpp"

#include "gbfr/core/c_api.h"
#include "gbfr/signatures.hpp"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace gbfr::app {

namespace {

const wchar_t* status_name(GbfrStatus s) {
    switch (s) {
    case GBFR_OK:                 return L"OK";
    case GBFR_ERR_INVALID_ARG:    return L"INVALID_ARG";
    case GBFR_ERR_NOT_FOUND:      return L"NOT_FOUND";
    case GBFR_ERR_ALREADY_INIT:   return L"ALREADY_INIT";
    case GBFR_ERR_NOT_INIT:       return L"NOT_INIT";
    case GBFR_ERR_NOT_AVAILABLE:  return L"NOT_AVAILABLE";
    case GBFR_ERR_PLATFORM:       return L"PLATFORM";
    case GBFR_ERR_OUT_OF_RANGE:   return L"OUT_OF_RANGE";
    }
    return L"?";
}

std::string to_utf8(const std::wstring& w) {
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                                      nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                        out.data(), n, nullptr, nullptr);
    return out;
}

bool parse_u64(const std::wstring& w, uint64_t& out) {
    if (w.empty()) return false;
    const wchar_t* p = w.c_str();
    if (w.size() > 2 && (w[0] == L'0' && (w[1] == L'x' || w[1] == L'X'))) p += 2;
    wchar_t* end = nullptr;
    unsigned long long v = std::wcstoull(p, &end, 16);
    if (!end || *end != L'\0') return false;
    out = static_cast<uint64_t>(v);
    return true;
}

bool parse_u32_dec_or_hex(const std::wstring& w, uint32_t& out) {
    if (w.empty()) return false;
    const wchar_t* p = w.c_str();
    int base = 10;
    if (w.size() > 2 && w[0] == L'0' && (w[1] == L'x' || w[1] == L'X')) { p += 2; base = 16; }
    wchar_t* end = nullptr;
    unsigned long long v = std::wcstoull(p, &end, base);
    if (!end || *end != L'\0') return false;
    out = static_cast<uint32_t>(v);
    return true;
}

struct Session {
    GbfrStatus status = GBFR_ERR_NOT_INIT;
    Session()  { status = gbfr_init(GBFR_ATTACH_EXTERNAL, nullptr, GBFR_INIT_NO_UI); }
    ~Session() { if (status == GBFR_OK) gbfr_shutdown(); }
    explicit operator bool() const { return status == GBFR_OK; }
};

void hexdump(uint64_t base, const std::vector<uint8_t>& data) {
    for (std::size_t off = 0; off < data.size(); off += 16) {
        std::wprintf(L"  %016llX  ",
                     static_cast<unsigned long long>(base + off));
        for (std::size_t i = 0; i < 16; ++i) {
            if (off + i < data.size()) {
                std::wprintf(L"%02X ", data[off + i]);
            } else {
                std::wprintf(L"   ");
            }
            if (i == 7) std::wprintf(L" ");
        }
        std::wprintf(L" ");
        for (std::size_t i = 0; i < 16 && off + i < data.size(); ++i) {
            const uint8_t c = data[off + i];
            std::wprintf(L"%c", (c >= 32 && c < 127) ? c : '.');
        }
        std::wprintf(L"\n");
    }
}

// ---- subcommands ----------------------------------------------------------

int sub_module_info() {
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    GbfrModuleInfo m{};
    const auto st = gbfr_debug_module_info(&m);
    if (st != GBFR_OK) { std::wprintf(L"module-info: %s\n", status_name(st)); return 1; }
    std::wprintf(L"base : 0x%016llX\n", static_cast<unsigned long long>(m.base));
    std::wprintf(L"size : 0x%016llX (%llu bytes)\n",
                 static_cast<unsigned long long>(m.size),
                 static_cast<unsigned long long>(m.size));
    return 0;
}

int sub_rva(const std::vector<std::wstring>& a) {
    if (a.size() != 1) { std::fwprintf(stderr, L"usage: debug rva <hex>\n"); return 1; }
    uint64_t rva = 0;
    if (!parse_u64(a[0], rva)) { std::fwprintf(stderr, L"bad hex\n"); return 1; }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    uint64_t abs = 0;
    const auto st = gbfr_debug_rva_to_abs(rva, &abs);
    if (st != GBFR_OK) { std::wprintf(L"rva: %s\n", status_name(st)); return 1; }
    std::wprintf(L"0x%016llX\n", static_cast<unsigned long long>(abs));
    return 0;
}

int sub_read_u32(const std::vector<std::wstring>& a) {
    if (a.size() != 1) { std::fwprintf(stderr, L"usage: debug read-u32 <hex>\n"); return 1; }
    uint64_t addr = 0; if (!parse_u64(a[0], addr)) { std::fwprintf(stderr, L"bad hex\n"); return 1; }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    uint32_t v = 0;
    const auto st = gbfr_debug_read_u32(addr, &v);
    if (st != GBFR_OK) { std::wprintf(L"read-u32: %s\n", status_name(st)); return 1; }
    std::wprintf(L"0x%08X  %u\n", v, v);
    return 0;
}

int sub_read_u64(const std::vector<std::wstring>& a) {
    if (a.size() != 1) { std::fwprintf(stderr, L"usage: debug read-u64 <hex>\n"); return 1; }
    uint64_t addr = 0; if (!parse_u64(a[0], addr)) { std::fwprintf(stderr, L"bad hex\n"); return 1; }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    uint64_t v = 0;
    const auto st = gbfr_debug_read_u64(addr, &v);
    if (st != GBFR_OK) { std::wprintf(L"read-u64: %s\n", status_name(st)); return 1; }
    std::wprintf(L"0x%016llX  %llu\n",
                 static_cast<unsigned long long>(v),
                 static_cast<unsigned long long>(v));
    return 0;
}

int sub_read_f32(const std::vector<std::wstring>& a) {
    if (a.size() != 1) { std::fwprintf(stderr, L"usage: debug read-f32 <hex>\n"); return 1; }
    uint64_t addr = 0; if (!parse_u64(a[0], addr)) { std::fwprintf(stderr, L"bad hex\n"); return 1; }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    float v = 0.0f;
    const auto st = gbfr_debug_read_f32(addr, &v);
    if (st != GBFR_OK) { std::wprintf(L"read-f32: %s\n", status_name(st)); return 1; }
    std::wprintf(L"%.6f\n", v);
    return 0;
}

int sub_read_bytes(const std::vector<std::wstring>& a) {
    if (a.size() != 2) { std::fwprintf(stderr, L"usage: debug read-bytes <hex_addr> <n>\n"); return 1; }
    uint64_t addr = 0; if (!parse_u64(a[0], addr)) { std::fwprintf(stderr, L"bad hex\n"); return 1; }
    uint32_t n = 0;   if (!parse_u32_dec_or_hex(a[1], n) || n == 0 || n > 4096) {
        std::fwprintf(stderr, L"bad count (1..4096)\n"); return 1;
    }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    std::vector<uint8_t> buf(n);
    const auto st = gbfr_debug_read_bytes(addr, buf.data(), n);
    if (st != GBFR_OK) { std::wprintf(L"read-bytes: %s\n", status_name(st)); return 1; }
    hexdump(addr, buf);
    return 0;
}

int sub_read_cstr(const std::vector<std::wstring>& a) {
    if (a.size() < 1 || a.size() > 2) {
        std::fwprintf(stderr, L"usage: debug read-cstr <hex_addr> [max=256]\n"); return 1;
    }
    uint64_t addr = 0; if (!parse_u64(a[0], addr)) { std::fwprintf(stderr, L"bad hex\n"); return 1; }
    uint32_t cap = 256;
    if (a.size() == 2 && (!parse_u32_dec_or_hex(a[1], cap) || cap == 0 || cap > 16384)) {
        std::fwprintf(stderr, L"bad max\n"); return 1;
    }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    std::vector<char> buf(cap + 1, 0);
    const auto st = gbfr_debug_read_cstr(addr, buf.data(), cap + 1);
    if (st != GBFR_OK) { std::wprintf(L"read-cstr: %s\n", status_name(st)); return 1; }
    std::wprintf(L"\"%hs\"\n", buf.data());
    return 0;
}

int sub_scan_qword(const std::vector<std::wstring>& a) {
    if (a.size() != 1) { std::fwprintf(stderr, L"usage: debug scan-qword <hex>\n"); return 1; }
    uint64_t v = 0; if (!parse_u64(a[0], v)) { std::fwprintf(stderr, L"bad hex\n"); return 1; }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    uint64_t addr = 0;
    const auto st = gbfr_debug_scan_qword(v, &addr);
    if (st != GBFR_OK) { std::wprintf(L"scan-qword: %s\n", status_name(st)); return 1; }
    std::wprintf(L"0x%016llX\n", static_cast<unsigned long long>(addr));
    return 0;
}

int sub_scan_qword_all(const std::vector<std::wstring>& a) {
    if (a.size() < 1 || a.size() > 2) {
        std::fwprintf(stderr, L"usage: debug scan-qword-all <hex> [cap=16]\n"); return 1;
    }
    uint64_t v = 0; if (!parse_u64(a[0], v)) { std::fwprintf(stderr, L"bad hex\n"); return 1; }
    uint32_t cap = 16;
    if (a.size() == 2 && (!parse_u32_dec_or_hex(a[1], cap) || cap == 0 || cap > 4096)) {
        std::fwprintf(stderr, L"bad cap\n"); return 1;
    }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    std::vector<uint64_t> out(cap);
    uint32_t count = 0;
    const auto st = gbfr_debug_scan_qword_all(v, out.data(), cap, &count);
    std::wprintf(L"%u match(es) (status=%s, showing up to %u)\n",
                 count, status_name(st), cap);
    for (uint32_t i = 0; i < count && i < cap; ++i) {
        std::wprintf(L"  [%u] 0x%016llX\n", i, static_cast<unsigned long long>(out[i]));
    }
    return 0;
}

int sub_scan_qword_process(const std::vector<std::wstring>& a) {
    if (a.size() < 1 || a.size() > 2) {
        std::fwprintf(stderr, L"usage: debug scan-qword-process <hex> [cap=16]\n"); return 1;
    }
    uint64_t v = 0; if (!parse_u64(a[0], v)) { std::fwprintf(stderr, L"bad hex\n"); return 1; }
    uint32_t cap = 16;
    if (a.size() == 2 && (!parse_u32_dec_or_hex(a[1], cap) || cap == 0 || cap > 1000000)) {
        std::fwprintf(stderr, L"bad cap\n"); return 1;
    }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    std::vector<uint64_t> out(cap);
    uint32_t count = 0;
    const auto st = gbfr_debug_scan_qword_in_process(v, out.data(), cap, &count);
    std::wprintf(L"%u match(es) (status=%s, showing up to %u)\n",
                 count, status_name(st), cap);
    for (uint32_t i = 0; i < count && i < cap; ++i) {
        std::wprintf(L"  [%u] 0x%016llX\n", i, static_cast<unsigned long long>(out[i]));
    }
    return 0;
}

int sub_scan_dword_process(const std::vector<std::wstring>& a) {
    if (a.size() < 1 || a.size() > 2) {
        std::fwprintf(stderr, L"usage: debug scan-dword-process <hex|dec> [cap=16]\n"); return 1;
    }
    uint32_t v = 0; if (!parse_u32_dec_or_hex(a[0], v)) { std::fwprintf(stderr, L"bad value\n"); return 1; }
    uint32_t cap = 16;
    if (a.size() == 2 && (!parse_u32_dec_or_hex(a[1], cap) || cap == 0 || cap > 4096)) {
        std::fwprintf(stderr, L"bad cap\n"); return 1;
    }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    std::vector<uint64_t> out(cap);
    uint32_t count = 0;
    const auto st = gbfr_debug_scan_dword_in_process(v, out.data(), cap, &count);
    std::wprintf(L"%u match(es) (status=%s, showing up to %u)\n",
                 count, status_name(st), cap);
    for (uint32_t i = 0; i < count && i < cap; ++i) {
        std::wprintf(L"  [%u] 0x%016llX\n", i, static_cast<unsigned long long>(out[i]));
    }
    return 0;
}

int sub_scan_pattern(const std::vector<std::wstring>& a) {
    if (a.size() != 1) {
        std::fwprintf(stderr, L"usage: debug scan-pattern \"<ida>\"\n"); return 1;
    }
    const std::string pat = to_utf8(a[0]);
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    uint64_t addr = 0;
    const auto st = gbfr_debug_scan_pattern(pat.c_str(), &addr);
    if (st != GBFR_OK) { std::wprintf(L"scan-pattern: %s\n", status_name(st)); return 1; }
    std::wprintf(L"0x%016llX\n", static_cast<unsigned long long>(addr));
    return 0;
}

int sub_scan_pattern_all(const std::vector<std::wstring>& a) {
    if (a.size() < 1 || a.size() > 2) {
        std::fwprintf(stderr, L"usage: debug scan-pattern-all \"<ida>\" [cap=16]\n"); return 1;
    }
    const std::string pat = to_utf8(a[0]);
    uint32_t cap = 16;
    if (a.size() == 2 && (!parse_u32_dec_or_hex(a[1], cap) || cap == 0 || cap > 4096)) {
        std::fwprintf(stderr, L"bad cap\n"); return 1;
    }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    std::vector<uint64_t> out(cap);
    uint32_t count = 0;
    const auto st = gbfr_debug_scan_pattern_all(pat.c_str(), out.data(), cap, &count);
    std::wprintf(L"%u match(es) (status=%s, showing up to %u)\n",
                 count, status_name(st), cap);
    for (uint32_t i = 0; i < count && i < cap; ++i) {
        std::wprintf(L"  [%u] 0x%016llX\n", i, static_cast<unsigned long long>(out[i]));
    }
    return 0;
}

int sub_lea_refs(const std::vector<std::wstring>& a) {
    if (a.size() < 1 || a.size() > 2) {
        std::fwprintf(stderr, L"usage: debug lea-refs <hex_addr> [cap=16]\n"); return 1;
    }
    uint64_t target = 0; if (!parse_u64(a[0], target)) { std::fwprintf(stderr, L"bad hex\n"); return 1; }
    uint32_t cap = 16;
    if (a.size() == 2 && (!parse_u32_dec_or_hex(a[1], cap) || cap == 0 || cap > 4096)) {
        std::fwprintf(stderr, L"bad cap\n"); return 1;
    }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    std::vector<uint64_t> out(cap);
    uint32_t count = 0;
    const auto st = gbfr_debug_find_lea_refs_to(target, out.data(), cap, &count);
    std::wprintf(L"%u match(es) (status=%s, showing up to %u)\n",
                 count, status_name(st), cap);
    for (uint32_t i = 0; i < count && i < cap; ++i) {
        std::wprintf(L"  [%u] 0x%016llX\n", i, static_cast<unsigned long long>(out[i]));
    }
    return 0;
}

// FNV-1a 32 of a UTF-8 string. Does not require an attached session.
int sub_hash(const std::vector<std::wstring>& a) {
    if (a.size() > 1) {
        std::fwprintf(stderr, L"usage: debug hash <utf8_string>\n"); return 1;
    }
    const std::string narrow = a.empty() ? std::string{} : to_utf8(a[0]);
    uint32_t h = 0;
    const auto st = gbfr_debug_hash32(narrow.c_str(), &h);
    if (st != GBFR_OK) {
        std::fwprintf(stderr, L"hash: %s\n", status_name(st));
        return 1;
    }
    std::wprintf(L"\"%hs\" -> 0x%08x\n", narrow.c_str(), h);
    return 0;
}

// Resolve a character_type hash via the baked Pl0000..Pl2300 table. Does
// not require an attached session.
int sub_lookup(const std::vector<std::wstring>& a) {
    if (a.size() != 1) {
        std::fwprintf(stderr, L"usage: debug lookup <hex_hash>\n"); return 1;
    }
    uint64_t v = 0;
    if (!parse_u64(a[0], v)) {
        std::fwprintf(stderr, L"bad hex\n"); return 1;
    }
    char asset_id[64] = {0};
    char name[64]     = {0};
    const auto st = gbfr_character_type_lookup(static_cast<uint32_t>(v),
                                               asset_id, name);
    if (st != GBFR_OK) {
        std::wprintf(L"0x%08x: %s\n",
                     static_cast<unsigned>(v), status_name(st));
        return 1;
    }
    std::wprintf(L"0x%08x -> %hs (%hs)\n",
                 static_cast<unsigned>(v), asset_id, name);
    return 0;
}

int sub_player_data_offset() {
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    uint32_t off = 0;
    const auto st = gbfr_debug_player_data_offset(&off);
    if (st != GBFR_OK) { std::wprintf(L"player-data-offset: %s\n", status_name(st)); return 1; }
    std::wprintf(L"player_data_offset: 0x%x (%u)\n", off, off);
    return 0;
}

int sub_app_main_loop() {
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    uint64_t addr = 0;
    const auto st = gbfr_debug_app_main_loop(&addr);
    if (st != GBFR_OK) { std::wprintf(L"app-main-loop: %s\n", status_name(st)); return 1; }
    std::wprintf(L"AppMainLoop @ 0x%016llX\n",
                 static_cast<unsigned long long>(addr));
    return 0;
}

// Resolve every known static instance / pointer-slot to live addresses and
// display them in one shot. Useful for sanity-checking that a particular
// game build's anchors still match the SDK's expected values.
int sub_instances() {
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }

    struct Anchor { const wchar_t* name; uint64_t rva; bool deref; };
    namespace sg = gbfr::signatures;
    const Anchor anchors[] = {
        // In-place statics: object lives at the RVA.
        {L"AppMainLoop            (inst)", sg::instance::kAppMainLoop,            false},
        {L"cCameraGame            (inst)", sg::instance::kCameraGame,             false},
        {L"cCameraApp #0          (inst)", sg::instance::kCameraApp0,             false},
        {L"cCameraApp #1          (inst)", sg::instance::kCameraApp1,             false},
        {L"cCameraApp #2          (inst)", sg::instance::kCameraApp2,             false},
        {L"cNpcAppearanceManager  (inst)", sg::instance::kNpcAppearanceManager,   false},
        {L"cOtManager             (inst)", sg::instance::kOtManager,              false},
        {L"hitflash::Manager      (inst)", sg::instance::kHitflashManager,        false},
        {L"photo::ProhibitManager (inst)", sg::instance::kPhotoProhibitManager,   false},
        {L"photo::ImpossibleMgr   (inst)", sg::instance::kPhotoImpossibleManager, false},
        // Pointer slots: dereference for the heap address.
        {L"PhotoManager           (slot)", sg::ptr_slot::kPhotoManager,            true},
        {L"NetworkSystemRpc       (slot)", sg::ptr_slot::kNetworkSystemRpcManager, true},
        {L"ProgressManager        (slot)", sg::ptr_slot::kProgressManager,         true},
        {L"stage::placement::Mgr  (slot)", sg::ptr_slot::kStagePlacementManager,   true},
        {L"NavimeshManager        (slot)", sg::ptr_slot::kNavimeshManager,         true},
        {L"cy::SceneObjectMgr     (slot)", sg::ptr_slot::kSceneObjectManager,      true},
        {L"cy::SubsurfaceShading  (slot)", sg::ptr_slot::kSubsurfaceShadingManager,true},
        {L"Hw::cUserManagerImpl   (slot)", sg::ptr_slot::kHwUserManagerImpl,       true},
        {L"Granite::UploadManager (slot)", sg::ptr_slot::kGraniteUploadManager,    true},
        {L"Sound::DefSoftCallMgr  (slot)", sg::ptr_slot::kSoundDefaultSoftCallManager, true},
        {L"EntityRegistry         (slot)", sg::ptr_slot::kEntityRegistry,          true},
        {L"SaveDataManager        (slot)", sg::ptr_slot::kSaveDataManager,         true},
    };

    for (const auto& a : anchors) {
        uint64_t abs_addr = 0;
        if (gbfr_debug_rva_to_abs(a.rva, &abs_addr) != GBFR_OK) {
            std::wprintf(L"%s : <rva failed>\n", a.name);
            continue;
        }
        if (a.deref) {
            uint64_t live = 0;
            const auto st = gbfr_debug_read_u64(abs_addr, &live);
            if (st == GBFR_OK) {
                std::wprintf(L"%s : *0x%016llX -> 0x%016llX\n",
                             a.name,
                             static_cast<unsigned long long>(abs_addr),
                             static_cast<unsigned long long>(live));
            } else {
                std::wprintf(L"%s : *0x%016llX -> %s\n",
                             a.name,
                             static_cast<unsigned long long>(abs_addr),
                             status_name(st));
            }
        } else {
            uint64_t vft = 0;
            const auto st = gbfr_debug_read_u64(abs_addr, &vft);
            if (st == GBFR_OK) {
                std::wprintf(L"%s : 0x%016llX  vft=0x%016llX\n",
                             a.name,
                             static_cast<unsigned long long>(abs_addr),
                             static_cast<unsigned long long>(vft));
            } else {
                std::wprintf(L"%s : 0x%016llX  vft=%s\n",
                             a.name,
                             static_cast<unsigned long long>(abs_addr),
                             status_name(st));
            }
        }
    }
    return 0;
}

int sub_player_info(const std::vector<std::wstring>& a) {
    if (a.size() != 1) {
        std::fwprintf(stderr, L"usage: debug player-info <hex_entity_ptr>\n"); return 1;
    }
    uint64_t ent = 0;
    if (!parse_u64(a[0], ent)) {
        std::fwprintf(stderr, L"bad hex\n"); return 1;
    }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }
    GbfrCombatInfo info{};
    const auto st = gbfr_combat_read_from_entity(ent, &info);
    if (st != GBFR_OK) {
        std::wprintf(L"player-info: %s\n", status_name(st));
        return 1;
    }
    std::wprintf(L"entity                  : 0x%016llX\n",
                 static_cast<unsigned long long>(ent));
    std::wprintf(L"level                   : %u\n", info.level);
    std::wprintf(L"hp / hp_max             : %.1f / %.1f\n", info.hp, info.hp_max);
    std::wprintf(L"critical_rate           : %.3f\n", info.critical_rate);
    return 0;
}

// CharaList layout probe: dump the first N bytes of each of the 40 entries.
// Helps to identify which dword inside each entry is the live key.
int sub_chara_list(const std::vector<std::wstring>& a) {
    uint32_t per_entry = 0x20;
    if (a.size() == 1 && (!parse_u32_dec_or_hex(a[0], per_entry) || per_entry == 0 || per_entry > 0x200)) {
        std::fwprintf(stderr, L"bad bytes-per-entry\n"); return 1;
    }
    Session s; if (!s) { std::fwprintf(stderr, L"attach: %s\n", status_name(s.status)); return 1; }

    uint64_t agg = 0;
    const auto st = gbfr_debug_save_aggregate(&agg);
    if (st != GBFR_OK) {
        std::wprintf(L"save-aggregate: %s\n", status_name(st));
        return 1;
    }
    const uint64_t list = agg + 0xd70ull; // signatures::offset::save_aggregate::kCharaList
    std::wprintf(L"aggregate  : 0x%016llX\n", static_cast<unsigned long long>(agg));
    std::wprintf(L"chara_list : 0x%016llX\n", static_cast<unsigned long long>(list));

    // Dump the header (16 bytes), then per-entry the first `per_entry` bytes.
    std::wprintf(L"\n-- list header (16 bytes) --\n");
    {
        std::vector<uint8_t> hdr(16);
        if (gbfr_debug_read_bytes(list, hdr.data(), 16) == GBFR_OK) hexdump(list, hdr);
    }

    constexpr uint32_t kMax    = 40;
    constexpr uint32_t kStride = 0x3120;
    std::vector<uint8_t> buf(per_entry);
    for (uint32_t i = 0; i < kMax; ++i) {
        const uint64_t addr = list + 0x10ull + static_cast<uint64_t>(i) * kStride;
        std::wprintf(L"\n-- entry[%u] @ 0x%016llX --\n",
                     i, static_cast<unsigned long long>(addr));
        if (gbfr_debug_read_bytes(addr, buf.data(), per_entry) != GBFR_OK) {
            std::wprintf(L"  (read failed)\n");
            continue;
        }
        hexdump(addr, buf);
    }
    return 0;
}

void print_help() {
    std::wprintf(
        L"debug subcommands (run as: GBFRMeme.exe -c debug <sub> [args]):\n"
        L"  module-info\n"
        L"  rva <hex_rva>\n"
        L"  read-u32   <hex_addr>\n"
        L"  read-u64   <hex_addr>\n"
        L"  read-f32   <hex_addr>\n"
        L"  read-bytes <hex_addr> <n>\n"
        L"  read-cstr  <hex_addr> [max=256]\n"
        L"  scan-qword <hex_value>\n"
        L"  scan-qword-all <hex_value> [cap=16]\n"
        L"  scan-qword-process <hex_value> [cap=16]\n"
        L"  scan-dword-process <hex_value> [cap=16]\n"
        L"  scan-pattern \"<ida pattern>\"\n"
        L"  scan-pattern-all \"<ida pattern>\" [cap=16]\n"
        L"  lea-refs   <hex_addr> [cap=16]\n"
        L"  chara-list [bytes_per_entry=0x20]\n"
        L"  hash <utf8_string>\n"
        L"  lookup <hex_hash>\n"
        L"  player-data-offset\n"
        L"  player-info <hex_entity_ptr>\n"
        L"  app-main-loop\n"
        L"  instances\n");
}

} // namespace

int cli_debug(const std::vector<std::wstring>& args) {
    if (args.empty() || args[0] == L"-h" || args[0] == L"--help" || args[0] == L"help") {
        print_help();
        return args.empty() ? 1 : 0;
    }
    const std::wstring& sub = args[0];
    const std::vector<std::wstring> rest(args.begin() + 1, args.end());

    if (sub == L"module-info")      return sub_module_info();
    if (sub == L"rva")              return sub_rva(rest);
    if (sub == L"read-u32")         return sub_read_u32(rest);
    if (sub == L"read-u64")         return sub_read_u64(rest);
    if (sub == L"read-f32")         return sub_read_f32(rest);
    if (sub == L"read-bytes")       return sub_read_bytes(rest);
    if (sub == L"read-cstr")        return sub_read_cstr(rest);
    if (sub == L"scan-qword")       return sub_scan_qword(rest);
    if (sub == L"scan-qword-all")   return sub_scan_qword_all(rest);
    if (sub == L"scan-qword-process") return sub_scan_qword_process(rest);
    if (sub == L"scan-dword-process") return sub_scan_dword_process(rest);
    if (sub == L"scan-pattern")     return sub_scan_pattern(rest);
    if (sub == L"scan-pattern-all") return sub_scan_pattern_all(rest);
    if (sub == L"lea-refs")         return sub_lea_refs(rest);
    if (sub == L"chara-list")       return sub_chara_list(rest);
    if (sub == L"hash")             return sub_hash(rest);
    if (sub == L"lookup")           return sub_lookup(rest);
    if (sub == L"player-data-offset") return sub_player_data_offset();
    if (sub == L"player-info")      return sub_player_info(rest);
    if (sub == L"app-main-loop")    return sub_app_main_loop();
    if (sub == L"instances")        return sub_instances();

    std::fwprintf(stderr, L"unknown debug subcommand: %s\n", sub.c_str());
    print_help();
    return 1;
}

} // namespace gbfr::app
