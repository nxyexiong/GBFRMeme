// SPDX-License-Identifier: MIT
// gbfr/common.hpp — primitive types and macros shared across the SDK.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace gbfr {

// 64-bit in-process pointer-sized address.
using Address = std::uintptr_t;

// Image base reported by Ghidra (`overview.txt`).
inline constexpr Address kDefaultImageBase = 0x140000000ULL;

// Marker macro for byte offsets that were not yet recovered from RTTI /
// decompilation. The argument is a documentation string. Resolves to a value
// of -1 that is obviously wrong if accidentally used at runtime.
//
// To replace one of these, drop the macro and write the literal offset, e.g.
//     static constexpr std::ptrdiff_t kLevelOffset = 0x40;
#define GBFR_TODO_OFFSET(reason) (static_cast<std::ptrdiff_t>(-1))

// Convert a Relative Virtual Address (recovered from Ghidra at image base
// 0x140000000) to an absolute in-process address given the module base.
[[nodiscard]] constexpr Address rva_to_absolute(Address image_base, Address rva) noexcept {
    return image_base + (rva - kDefaultImageBase);
}

// 32-bit FNV-style string hash used everywhere by the cyan engine as keys
// into save data, asset tables, and the singleton registry.
//
// NOTE: the exact polynomial / seed combination has NOT been confirmed by
// reverse engineering. This is the standard FNV-1a 32-bit and matches the
// shape that `cyan::string_hash32` exhibits, but DO verify against a known
// (string -> hash) pair from the game before relying on it.
class StringHash32 {
public:
    constexpr StringHash32() noexcept = default;
    constexpr explicit StringHash32(std::uint32_t v) noexcept : m_value(v) {}
    constexpr explicit StringHash32(std::string_view s) noexcept : m_value(hash(s)) {}

    [[nodiscard]] constexpr std::uint32_t value() const noexcept { return m_value; }

    friend constexpr bool operator==(StringHash32 a, StringHash32 b) noexcept { return a.m_value == b.m_value; }
    friend constexpr bool operator!=(StringHash32 a, StringHash32 b) noexcept { return a.m_value != b.m_value; }

private:
    static constexpr std::uint32_t kFnv1aSeed = 0x811C9DC5u;
    static constexpr std::uint32_t kFnv1aPrime = 0x01000193u;

    static constexpr std::uint32_t hash(std::string_view s) noexcept {
        std::uint32_t h = kFnv1aSeed;
        for (unsigned char c : s) {
            h ^= c;
            h *= kFnv1aPrime;
        }
        return h;
    }

    std::uint32_t m_value{0};
};

// Compile-time literal: `"foo"_h32`.
constexpr StringHash32 operator""_h32(const char* s, std::size_t n) noexcept {
    return StringHash32(std::string_view{s, n});
}

// 4-byte enum sized to match the game's `eObjId` and similar tags.
enum class ObjId : std::uint32_t {};

// Opaque 64-bit packed handle (generation + index) used by the game to refer
// to live `Entity` instances.
class EntityHandle {
public:
    constexpr EntityHandle() noexcept = default;
    constexpr explicit EntityHandle(std::uint64_t v) noexcept : m_raw(v) {}

    [[nodiscard]] constexpr std::uint64_t raw() const noexcept { return m_raw; }
    [[nodiscard]] constexpr bool is_valid() const noexcept { return m_raw != 0; }

    friend constexpr bool operator==(EntityHandle a, EntityHandle b) noexcept { return a.m_raw == b.m_raw; }
    friend constexpr bool operator!=(EntityHandle a, EntityHandle b) noexcept { return a.m_raw != b.m_raw; }

private:
    std::uint64_t m_raw{0};
};

} // namespace gbfr
