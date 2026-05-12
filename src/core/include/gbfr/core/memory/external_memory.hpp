// SPDX-License-Identifier: MIT
// gbfr/core/memory/external_memory.hpp — out-of-process IMemory backend.
//
// Uses Win32 ReadProcessMemory / WriteProcessMemory against a foreign PID.
// Module base is discovered by enumerating the target's loaded modules.
#pragma once

#include "gbfr/memory.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace gbfr::core {

class ExternalMemory final : public gbfr::IMemory {
public:
    // Open a handle to `pid`, look up `module_name` in its module list, and
    // return a ready-to-use backend. Returns nullptr if the process cannot
    // be opened with PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION
    // |PROCESS_QUERY_INFORMATION, or if the module is not loaded.
    [[nodiscard]] static std::unique_ptr<ExternalMemory> attach(
        std::uint32_t pid,
        const std::wstring& module_name = L"granblue_fantasy_relink.exe");

    // Convenience: enumerate processes for the first one whose main module
    // name matches `executable_name` (case-insensitive), then attach.
    [[nodiscard]] static std::unique_ptr<ExternalMemory> attach_by_name(
        const std::wstring& executable_name = L"granblue_fantasy_relink.exe");

    ~ExternalMemory() override;

    ExternalMemory(const ExternalMemory&) = delete;
    ExternalMemory& operator=(const ExternalMemory&) = delete;

    [[nodiscard]] bool read(gbfr::Address addr, void* out, std::size_t size) const override;
    [[nodiscard]] bool write(gbfr::Address addr, const void* in, std::size_t size) override;
    [[nodiscard]] gbfr::Address module_base() const override { return m_module_base; }

    [[nodiscard]] std::uint32_t pid() const noexcept { return m_pid; }
    [[nodiscard]] std::size_t   module_size() const noexcept { return m_module_size; }
    [[nodiscard]] void*         native_handle() const noexcept { return m_handle; }

private:
    ExternalMemory(std::uint32_t pid, void* handle,
                   gbfr::Address module_base, std::size_t module_size) noexcept;

    std::uint32_t m_pid{0};
    void*         m_handle{nullptr}; // HANDLE on Windows
    gbfr::Address m_module_base{0};
    std::size_t   m_module_size{0};
};

} // namespace gbfr::core
