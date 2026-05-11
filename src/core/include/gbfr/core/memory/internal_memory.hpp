// SPDX-License-Identifier: MIT
// gbfr/core/memory/internal_memory.hpp — in-process IMemory backend.
//
// Addresses ARE pointers in the current process; reads are memcpy.
// Use when this code is loaded into the game (e.g. as an injected DLL).
#pragma once

#include "gbfr/memory.hpp"

namespace gbfr::core {

class InternalMemory final : public gbfr::IMemory {
public:
    explicit InternalMemory(gbfr::Address module_base) noexcept;

    [[nodiscard]] bool read(gbfr::Address addr, void* out, std::size_t size) const override;
    [[nodiscard]] bool write(gbfr::Address addr, const void* in, std::size_t size) override;
    [[nodiscard]] gbfr::Address module_base() const override { return m_module_base; }

private:
    gbfr::Address m_module_base;
};

} // namespace gbfr::core
