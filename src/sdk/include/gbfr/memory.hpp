// SPDX-License-Identifier: MIT
// gbfr/memory.hpp — pluggable memory access hook.
//
// The SDK is host-agnostic: it does not know whether memory is read in-process
// (direct pointers) or out-of-process (ReadProcessMemory / ptrace / network
// debug protocol). Concrete `IMemory` implementations live in `gbfr_core`.
#pragma once

#include "common.hpp"

#include <type_traits>

namespace gbfr {

// Abstract memory backend. All addresses are absolute pointers in the target
// (which may be the current process or a remote one).
class IMemory {
public:
    virtual ~IMemory() = default;

    // Read `size` bytes starting at `addr` into `out`. Returns true on
    // success. May fail if the page is not committed / readable.
    [[nodiscard]] virtual bool read(Address addr, void* out, std::size_t size) const = 0;

    // Write `size` bytes from `in` to `addr`. Returns true on success.
    [[nodiscard]] virtual bool write(Address addr, const void* in, std::size_t size) = 0;

    // Module base of `granblue_fantasy_relink.exe` in the target. Used to
    // translate RVAs (encoded at the design-time image base 0x140000000)
    // into runtime addresses.
    [[nodiscard]] virtual Address module_base() const = 0;
};

// Typed read/write convenience helpers.
class Memory {
public:
    template <class T>
    [[nodiscard]] static T read(const IMemory& mem, Address addr) {
        static_assert(std::is_trivially_copyable_v<T>, "Memory::read requires trivially copyable T");
        T out{};
        (void)mem.read(addr, &out, sizeof(T));
        return out;
    }

    template <class T>
    static bool write(IMemory& mem, Address addr, const T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "Memory::write requires trivially copyable T");
        return mem.write(addr, &value, sizeof(T));
    }
};

} // namespace gbfr
