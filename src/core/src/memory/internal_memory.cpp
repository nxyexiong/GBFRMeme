// SPDX-License-Identifier: MIT
#include "gbfr/core/memory/internal_memory.hpp"

#include <cstring>

namespace gbfr::core {

InternalMemory::InternalMemory(gbfr::Address module_base) noexcept
    : m_module_base(module_base) {}

bool InternalMemory::read(gbfr::Address addr, void* out, std::size_t size) const {
    if (addr == 0 || out == nullptr) return false;
    std::memcpy(out, reinterpret_cast<const void*>(addr), size);
    return true;
}

bool InternalMemory::write(gbfr::Address addr, const void* in, std::size_t size) {
    if (addr == 0 || in == nullptr) return false;
    std::memcpy(reinterpret_cast<void*>(addr), in, size);
    return true;
}

} // namespace gbfr::core
