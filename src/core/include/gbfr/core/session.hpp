// SPDX-License-Identifier: MIT
// gbfr/core/session.hpp — high-level glue: owns a memory backend, wires up
// the SDK `Game` facade against the running module, and stores discovered
// addresses for the lifetime of the program.
#pragma once

#include "gbfr/core/process_attach.hpp"
#include "gbfr/game.hpp"
#include "gbfr/memory.hpp"

#include <memory>
#include <optional>
#include <unordered_map>

namespace gbfr::core {

class Session {
public:
    // Locate the game module in this process and build a Session bound to
    // it. Returns std::nullopt if the module is not loaded.
    [[nodiscard]] static std::optional<Session> attach_in_process();

    // Open the first running `granblue_fantasy_relink.exe` process and
    // build a Session wired to an ExternalMemory backend.
    [[nodiscard]] static std::optional<Session> attach_external_by_name(
        const std::wstring& executable_name = L"granblue_fantasy_relink.exe");

    Session(Session&&) noexcept = default;
    Session& operator=(Session&&) noexcept = default;
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    [[nodiscard]] const ModuleInfo& module_info() const noexcept { return m_module; }
    [[nodiscard]] gbfr::IMemory&    memory()      const noexcept { return *m_memory; }
    [[nodiscard]] gbfr::Game&       game()        noexcept       { return *m_game; }
    [[nodiscard]] const gbfr::Game& game()        const noexcept { return *m_game; }

    // OS process handle compatible with VirtualQueryEx / ReadProcessMemory.
    // For an in-process attach this is the current-process pseudo handle
    // (which works as a HANDLE for those APIs). For external attach this
    // is the OpenProcess result owned by `ExternalMemory`.
    [[nodiscard]] void* process_handle() const noexcept;

    // Attempt to populate singleton instance addresses by scanning for the
    // `lea` references to each manager's class-name string in `.rdata`
    // (which is how `cyan::Singleton<T>::registerInstance` looks up the
    // slot). Best-effort: returns the number of singletons resolved.
    std::size_t resolve_singletons_via_name_strings();

    // Translate an SDK-known RVA (design-time image base 0x140000000) into
    // an absolute address in the live process.
    [[nodiscard]] Address rva(Address rva_value) const noexcept;

    // Live address of the parent save-data aggregate. Discovered on first
    // call by scanning for one known list vftable and subtracting the
    // verified offset; cached thereafter. Returns 0 if not found.
    [[nodiscard]] Address save_aggregate_address();

    // Offset of the `PlayerStats` struct relative to a player Entity.
    // Game 2.0 resolves the property through a runtime table, so this is a
    // build-specific verified offset rather than a byte-pattern result.
    [[nodiscard]] std::uint32_t player_data_offset();

    // Scan the process for any live player Entity by looking for the known
    // playable-character vftables. First call walks the address space
    // (~80s); subsequent calls return the cached address. Returns the live
    // Entity pointer or 0 if no player is currently spawned.
    [[nodiscard]] Address find_local_player();

    // Locate a live `sys::data::*List` instance by scanning for its vftable
    // pointer. Cached on success (the lists are heap-allocated but stable
    // for the lifetime of the process). Returns 0 if not found.
    [[nodiscard]] Address find_save_list(Address vftable_rva);

    // Locate the active save slot's "user block" (rupies, mastery points,
    // player name, etc.). Discovered by scanning every `SaveDataUnit<int,1>`
    // instance and finding the unique pair whose `src_ptr` values differ by
    // exactly `signatures::offset::user_save_block::kMasteryPointsU32`
    // (0x68). First call walks the process (~80s); subsequent calls return
    // the cached base. Returns 0 if not found.
    [[nodiscard]] Address find_user_save_block();

private:
    Session(ModuleInfo info, std::unique_ptr<gbfr::IMemory> mem) noexcept;

    ModuleInfo                     m_module;
    std::unique_ptr<gbfr::IMemory> m_memory;
    std::unique_ptr<gbfr::Game>    m_game;
    Address                        m_save_aggregate{0};
    Address                        m_local_player{0};
    Address                        m_user_save_block{0};
    std::unordered_map<Address, Address> m_list_cache;
};

} // namespace gbfr::core
