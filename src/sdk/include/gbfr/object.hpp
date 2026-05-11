// SPDX-License-Identifier: MIT
// gbfr/object.hpp — base class for every game-side object wrapper.
#pragma once

#include "common.hpp"
#include "memory.hpp"

namespace gbfr {

// Wraps a single in-target object at a fixed address. Subclasses expose
// typed getters that compute `m_address + offset` and dispatch through
// `m_memory`.
class GameObject {
public:
    GameObject() noexcept = default;
    GameObject(IMemory& mem, Address addr) noexcept : m_memory(&mem), m_address(addr) {}

    [[nodiscard]] Address address() const noexcept { return m_address; }
    [[nodiscard]] bool is_valid() const noexcept { return m_memory != nullptr && m_address != 0; }
    [[nodiscard]] IMemory& memory() const noexcept { return *m_memory; }

    // Read the vftable pointer stored at `m_address + 0`. Returns 0 if the
    // wrapper is invalid or the read fails.
    [[nodiscard]] Address vftable() const {
        if (!is_valid()) return 0;
        return read<Address>(0);
    }

protected:
    template <class T>
    [[nodiscard]] T read(std::ptrdiff_t offset) const {
        return Memory::read<T>(*m_memory, m_address + static_cast<Address>(offset));
    }

    template <class T>
    bool write(std::ptrdiff_t offset, const T& value) const {
        return Memory::write<T>(*m_memory, m_address + static_cast<Address>(offset), value);
    }

    // Convenience: build another wrapper from a member pointer stored at
    // `offset` inside this object.
    template <class Wrapper>
    [[nodiscard]] Wrapper read_object_ptr(std::ptrdiff_t offset) const {
        Address ptr = read<Address>(offset);
        return ptr ? Wrapper{*m_memory, ptr} : Wrapper{};
    }

    // Translate an RVA (recovered from Ghidra at the design-time image base
    // of 0x140000000) into an absolute address in the target process.
    [[nodiscard]] Address rva(Address rva_value) const noexcept {
        return rva_to_absolute(m_memory->module_base(), rva_value);
    }

private:
    IMemory* m_memory{nullptr};
    Address  m_address{0};
};

} // namespace gbfr
