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

// 32-bit string hash used everywhere by the cyan engine as keys
// into save data, asset tables, and the singleton registry.
//
// Algorithm: a "custom" xxHash32 variant. The standard derivation
// `v1 = seed + P1 + P2; v2 = seed + P2; v3 = seed; v4 = seed - P1`
// is NOT used. Instead the 4 SIMD-lane initial accumulators and the
// short-input scalar initial are independent baked constants:
//
//   scalar (len < 16): h32 = 0x178A54A4
//   SIMD lane 0..3   : 0x2557311B, 0x871FB76A, 0x0133ECF3, 0x62FC7342
//
// All recovered from FUN_1402260e0 in granblue_fantasy_relink.exe and the
// .rdata accumulator at 0x1447e61f0. Matches Nenkai's XXHash32Custom.cs.
// Sanity check: hash("") == 0x887AE0B0.
class StringHash32 {
public:
    constexpr StringHash32() noexcept = default;
    constexpr explicit StringHash32(std::uint32_t v) noexcept : m_value(v) {}
    constexpr explicit StringHash32(std::string_view s) noexcept : m_value(hash(s)) {}

    [[nodiscard]] constexpr std::uint32_t value() const noexcept { return m_value; }

    friend constexpr bool operator==(StringHash32 a, StringHash32 b) noexcept { return a.m_value == b.m_value; }
    friend constexpr bool operator!=(StringHash32 a, StringHash32 b) noexcept { return a.m_value != b.m_value; }

private:
    static constexpr std::uint32_t kP1 = 0x9E3779B1u;
    static constexpr std::uint32_t kP2 = 0x85EBCA77u;
    static constexpr std::uint32_t kP3 = 0xC2B2AE3Du;
    static constexpr std::uint32_t kP4 = 0x27D4EB2Fu;
    static constexpr std::uint32_t kP5 = 0x165667B1u;

    static constexpr std::uint32_t kInitScalar = 0x178A54A4u;
    static constexpr std::uint32_t kInitV1     = 0x2557311Bu;
    static constexpr std::uint32_t kInitV2     = 0x871FB76Au;
    static constexpr std::uint32_t kInitV3     = 0x0133ECF3u;
    static constexpr std::uint32_t kInitV4     = 0x62FC7342u;

    static constexpr std::uint32_t rotl(std::uint32_t x, int n) noexcept {
        return (x << n) | (x >> (32 - n));
    }

    static constexpr std::uint32_t round(std::uint32_t acc, std::uint32_t input) noexcept {
        return rotl(acc + input * kP2, 13) * kP1;
    }

    static constexpr std::uint32_t hash(std::string_view s) noexcept {
        const std::size_t n = s.size();
        std::size_t i = 0;
        std::uint32_t h = kInitScalar;

        auto read_u32_le_at = [&](std::size_t off) -> std::uint32_t {
            return  (static_cast<std::uint32_t>(static_cast<unsigned char>(s[off + 0])))
                 | (static_cast<std::uint32_t>(static_cast<unsigned char>(s[off + 1])) <<  8)
                 | (static_cast<std::uint32_t>(static_cast<unsigned char>(s[off + 2])) << 16)
                 | (static_cast<std::uint32_t>(static_cast<unsigned char>(s[off + 3])) << 24);
        };

        if (n >= 16) {
            std::uint32_t v1 = kInitV1;
            std::uint32_t v2 = kInitV2;
            std::uint32_t v3 = kInitV3;
            std::uint32_t v4 = kInitV4;
            const std::size_t limit = n - 16;
            do {
                v1 = round(v1, read_u32_le_at(i));
                v2 = round(v2, read_u32_le_at(i +  4));
                v3 = round(v3, read_u32_le_at(i +  8));
                v4 = round(v4, read_u32_le_at(i + 12));
                i += 16;
            } while (i <= limit);
            h = rotl(v1, 1) + rotl(v2, 7) + rotl(v3, 12) + rotl(v4, 18);
        }
        h += static_cast<std::uint32_t>(n);

        while (i + 4 <= n) {
            h = rotl(h + read_u32_le_at(i) * kP3, 17) * kP4;
            i += 4;
        }
        while (i < n) {
            h = rotl(h + static_cast<std::uint32_t>(static_cast<unsigned char>(s[i])) * kP5, 11) * kP1;
            ++i;
        }

        h ^= h >> 15; h *= kP2;
        h ^= h >> 13; h *= kP3;
        h ^= h >> 16;
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
