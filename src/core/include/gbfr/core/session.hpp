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

    // Attempt to populate singleton instance addresses by scanning for the
    // `lea` references to each manager's class-name string in `.rdata`
    // (which is how `cyan::Singleton<T>::registerInstance` looks up the
    // slot). Best-effort: returns the number of singletons resolved.
    std::size_t resolve_singletons_via_name_strings();

    // Translate an SDK-known RVA (design-time image base 0x140000000) into
    // an absolute address in the live process.
    [[nodiscard]] Address rva(Address rva_value) const noexcept;

private:
    Session(ModuleInfo info, std::unique_ptr<gbfr::IMemory> mem) noexcept;

    ModuleInfo                     m_module;
    std::unique_ptr<gbfr::IMemory> m_memory;
    std::unique_ptr<gbfr::Game>    m_game;
};

} // namespace gbfr::core
