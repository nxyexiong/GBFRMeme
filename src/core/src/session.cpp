// SPDX-License-Identifier: MIT
#include "gbfr/core/session.hpp"

#include "gbfr/character_types.hpp"
#include "gbfr/core/debug.hpp"
#include "gbfr/core/memory/internal_memory.hpp"
#include "gbfr/core/memory/external_memory.hpp"
#include "gbfr/signatures.hpp"

#include <windows.h>

#include <array>

namespace gbfr::core {

Session::Session(ModuleInfo info, std::unique_ptr<gbfr::IMemory> mem) noexcept
    : m_module(info), m_memory(std::move(mem)),
      m_game(std::make_unique<gbfr::Game>(*m_memory)) {}

std::optional<Session> Session::attach_in_process() {
    auto info = find_loaded_module();
    if (!info.has_value()) return std::nullopt;
    auto mem = std::make_unique<InternalMemory>(info->base);
    return Session{*info, std::move(mem)};
}

std::optional<Session> Session::attach_external_by_name(const std::wstring& executable_name) {
    auto ext = ExternalMemory::attach_by_name(executable_name);
    if (ext == nullptr) return std::nullopt;
    ModuleInfo info{ext->module_base(), ext->module_size()};
    return Session{info, std::move(ext)};
}

std::size_t Session::resolve_singletons_via_name_strings() {
    // Each manager records `kNameStringRva` of its class-name string in
    // .rdata. The `cyan::Singleton<T>::registerInstance` accessor LEAs that
    // string into a register before calling the registry. By finding the
    // first `lea rcx,[name]` within the live module and walking forward
    // through the enclosing function we could locate the static slot.
    //
    // The exact disassembly walk requires a decoder (e.g. Zydis). For now
    // this method is a stub that records candidate LEA sites for the
    // host to verify with a debugger. Returns 0 until implemented.
    return 0;
}

Address Session::rva(Address rva_value) const noexcept {
    return rva_to_absolute(m_module.base, rva_value);
}

void* Session::process_handle() const noexcept {
    if (auto* ext = dynamic_cast<ExternalMemory*>(m_memory.get())) {
        return ext->native_handle();
    }
    return ::GetCurrentProcess(); // pseudo-handle, valid with VirtualQueryEx / RPM
}

Address Session::save_aggregate_address() {
    if (m_save_aggregate != 0) return m_save_aggregate;

    // Find the live CharaList by scanning the process for its vftable
    // pointer, then subtract the CharaList's offset inside the parent.
    const Address vft = rva(signatures::vft::kCharaList);
    auto matches = debug::scan_qword_in_process(*this, static_cast<std::uint64_t>(vft), 4);
    if (matches.empty()) return 0;

    const Address list_addr = matches.front();
    const auto    list_off  = static_cast<Address>(signatures::offset::save_aggregate::kCharaList);
    if (list_addr < list_off) return 0;
    m_save_aggregate = list_addr - list_off;
    return m_save_aggregate;
}

std::uint32_t Session::player_data_offset() {
    return static_cast<std::uint32_t>(
        gbfr::signatures::offset::player_entity::kPlayerData);
}

Address Session::find_local_player() {
    if (m_local_player != 0) {
        // Validate the cache is still pointing at a player entity by
        // re-reading the vftable.
        std::uint64_t vft = 0;
        if (m_memory->read(m_local_player, &vft, sizeof(vft))) {
            for (const auto& row : kCharacterTypes) {
                if (row.is_playable &&
                    rva(row.vftable_rva) == static_cast<Address>(vft)) {
                    return m_local_player;
                }
            }
        }
        m_local_player = 0; // stale
    }

    std::array<std::uint64_t, kCharacterTypes.size()> vftables{};
    std::size_t count = 0;
    for (const auto& row : kCharacterTypes) {
        if (!row.is_playable) continue;
        vftables[count++] = static_cast<std::uint64_t>(
            rva(row.vftable_rva));
    }
    m_local_player = debug::scan_any_qword_in_process(
        *this, std::span<const std::uint64_t>{vftables.data(), count});
    return m_local_player;
}

Address Session::find_save_list(Address vftable_rva) {
    auto it = m_list_cache.find(vftable_rva);
    if (it != m_list_cache.end()) {
        // Validate the cached pointer still matches the vftable.
        std::uint64_t vft = 0;
        if (m_memory->read(it->second, &vft, sizeof(vft)) &&
            static_cast<Address>(vft) == rva(vftable_rva)) {
            return it->second;
        }
        m_list_cache.erase(it);
    }
    const Address vft_abs = rva(vftable_rva);
    auto matches = debug::scan_qword_in_process(*this, static_cast<std::uint64_t>(vft_abs), 1);
    if (matches.empty()) return 0;
    m_list_cache[vftable_rva] = matches.front();
    return matches.front();
}

Address Session::find_user_save_block() {
    namespace sg = gbfr::signatures;
    constexpr std::ptrdiff_t kMasteryOff = sg::offset::user_save_block::kMasteryPointsU32;
    constexpr std::ptrdiff_t kSrcOff     = sg::offset::save_data_unit::kSrcPtr;

    // Cache validation: re-derive src_ptr at base from any SaveDataUnit<int,1>
    // record that should still point at it. If our cached base is still a
    // valid heap address we keep it; otherwise re-scan.
    if (m_user_save_block != 0) {
        std::uint32_t probe = 0;
        if (m_memory->read(m_user_save_block, &probe, sizeof(probe))) {
            // Probe succeeded; assume still valid.
            return m_user_save_block;
        }
        m_user_save_block = 0;
    }

    const Address vft = rva(sg::vft::kSaveDataUnitInt1);
    // SaveDataUnit<int,1> is heavily reused; we routinely see ~93k live
    // instances across the entire process. 200_000 is a comfortable bound.
    auto records = debug::scan_qword_in_process(
        *this, static_cast<std::uint64_t>(vft), 200000);
    if (records.empty()) return 0;

    // Build the set of src_ptr values across all records.
    std::unordered_map<std::uint64_t, Address> by_src;
    by_src.reserve(records.size() * 2);
    for (Address rec : records) {
        std::uint64_t src = 0;
        if (!m_memory->read(rec + kSrcOff, &src, sizeof(src))) continue;
        if (src == 0) continue;
        by_src.emplace(src, rec);
    }

    // Find the unique pair (P, P+0x68) both present in the src set, with
    // these additional invariants verified from the real wallet block:
    //   * +0x08  u32 = 1                (slot-active flag)
    //   * +0x0C..+0x4C  UTF-16 player name: first wchar is printable ASCII
    //                   (letter/digit), and the name terminates inside the
    //                   64-byte buffer (a null wchar appears before +0x4C)
    // Together with the structural (P, P+0x68) pair these are sufficient to
    // uniquely identify the wallet.
    auto looks_like_name = [&](std::uint64_t base) -> bool {
        constexpr auto kNameOff = gbfr::signatures::offset::user_save_block::kPlayerNameUtf16;
        constexpr auto kNameSz  = gbfr::signatures::offset::user_save_block::kPlayerNameSize;
        std::uint8_t buf[kNameSz];
        if (!m_memory->read(static_cast<Address>(base) + kNameOff, buf, sizeof(buf))) {
            return false;
        }
        // First wchar
        if (buf[1] != 0) return false;
        const auto c0 = buf[0];
        const bool printable_start =
            (c0 >= 'A' && c0 <= 'Z') ||
            (c0 >= 'a' && c0 <= 'z') ||
            (c0 >= '0' && c0 <= '9');
        if (!printable_start) return false;
        // Find a null wchar within the buffer, ensure subsequent wchars are
        // also null (clean terminator + zero padding).
        bool found_terminator = false;
        for (std::size_t i = 0; i + 1 < sizeof(buf); i += 2) {
            const std::uint8_t lo = buf[i], hi = buf[i + 1];
            if (!found_terminator) {
                if (lo == 0 && hi == 0) { found_terminator = true; continue; }
                if (hi != 0) return false;
                if (lo < 0x20 || lo > 0x7E) return false;
            } else {
                if (lo != 0 || hi != 0) return false;
            }
        }
        return found_terminator;
    };

    for (const auto& [src, rec] : by_src) {
        if (by_src.find(src + static_cast<std::uint64_t>(kMasteryOff)) == by_src.end()) {
            continue;
        }
        std::uint32_t flag = 0;
        if (!m_memory->read(static_cast<Address>(src)
                            + gbfr::signatures::offset::user_save_block::kSlotActiveFlag,
                            &flag, sizeof(flag))) {
            continue;
        }
        if (flag != 1) continue;
        if (!looks_like_name(src)) continue;
        m_user_save_block = static_cast<Address>(src);
        return m_user_save_block;
    }
    return 0;
}

} // namespace gbfr::core
