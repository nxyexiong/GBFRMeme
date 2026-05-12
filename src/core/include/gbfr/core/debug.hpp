// SPDX-License-Identifier: MIT
// gbfr/core/debug.hpp — low-level introspection helpers for reverse engineering
// the live game. Operates on a `Session`'s `IMemory` backend.
//
// Not part of the stable C ABI. Exposed only so the in-process / CLI debugger
// surface can wrap these helpers.
#pragma once

#include "gbfr/common.hpp"
#include "gbfr/core/session.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gbfr::core::debug {

// ---------- Module info ----------------------------------------------------

[[nodiscard]] Address     module_base(const Session& s) noexcept;
[[nodiscard]] std::size_t module_size(const Session& s) noexcept;

// Translate a Ghidra-style RVA (image base 0x140000000) to a live absolute
// address in the attached process.
[[nodiscard]] Address rva_to_abs(const Session& s, Address rva) noexcept;

// ---------- Primitive reads -----------------------------------------------

[[nodiscard]] bool read(const Session& s, Address addr, void* out, std::size_t n);

template <class T>
[[nodiscard]] std::optional<T> read_typed(const Session& s, Address addr) {
    static_assert(std::is_trivially_copyable_v<T>);
    T v{};
    return read(s, addr, &v, sizeof(T)) ? std::optional<T>{v} : std::nullopt;
}

// NUL-terminated UTF-8 / ASCII string. Reads up to `max_n` bytes; stops at the
// first NUL. Returns empty string on read failure.
[[nodiscard]] std::string read_cstr(const Session& s, Address addr,
                                    std::size_t max_n = 256);

// ---------- Scans ----------------------------------------------------------

// First aligned occurrence of `value` (8 bytes, little-endian) inside the
// loaded module. Returns 0 if not found.
[[nodiscard]] Address scan_qword(const Session& s, std::uint64_t value);

// All aligned occurrences (caps at `max_results`).
[[nodiscard]] std::vector<Address> scan_qword_all(const Session& s,
                                                  std::uint64_t value,
                                                  std::size_t max_results = 64);

// Like scan_qword_all, but walks ALL committed readable regions of the
// attached process (heap, stacks, mapped images, .data, etc.) via
// VirtualQueryEx. Use this to find live instances by their vftable pointer.
// `max_results` caps the output; reads happen in 1 MiB chunks.
[[nodiscard]] std::vector<Address> scan_qword_in_process(
    const Session& s, std::uint64_t value, std::size_t max_results = 64);

// 4-byte little-endian variant. Aligned at 4 bytes. Useful for finding
// fields whose surrounding bytes aren't predictable.
[[nodiscard]] std::vector<Address> scan_dword_in_process(
    const Session& s, std::uint32_t value, std::size_t max_results = 64);

// IDA-style hex pattern: "48 8B ?? E8 ?? ?? ?? ??". Returns first match
// address, or 0 if not found.
[[nodiscard]] Address scan_pattern(const Session& s, std::string_view pattern);

// Several matches. `max_results` caps the output.
[[nodiscard]] std::vector<Address> scan_pattern_all(const Session& s,
                                                    std::string_view pattern,
                                                    std::size_t max_results = 32);

// ---------- Code refs ------------------------------------------------------

// Find every `lea rXX, [rip+disp32]` instruction in `.text` whose effective
// address equals `target`. Useful for locating callers of e.g.
// `cyan::Singleton<T>::installInstance(this)` that LEA the manager's class
// name string into rcx.
//
// Heuristic: scans for the 4-byte LEA pattern (`48 8D ?? ??`) and checks the
// 32-bit signed displacement. Returns the address of the LEA opcode.
[[nodiscard]] std::vector<Address> find_lea_refs_to(const Session& s,
                                                    Address target,
                                                    std::size_t max_results = 32);

} // namespace gbfr::core::debug
