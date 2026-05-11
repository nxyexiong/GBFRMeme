// SPDX-License-Identifier: MIT
#include "gbfr/core/signatures.hpp"

#include <cctype>
#include <cstring>

namespace gbfr::core {

namespace {
constexpr int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return -1;
}
} // namespace

Pattern compile_pattern(std::string_view text) {
    Pattern out;
    std::size_t i = 0;
    while (i < text.size()) {
        // Skip whitespace.
        while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
        if (i >= text.size()) break;
        // Expect a 2-char token.
        if (i + 1 >= text.size()) break;
        char a = text[i], b = text[i + 1];
        i += 2;
        if (a == '?' && b == '?') {
            out.bytes.push_back(0x00);
            out.mask.push_back(false);
        } else {
            int hi = hex_digit(a), lo = hex_digit(b);
            if (hi < 0 || lo < 0) continue;
            out.bytes.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
            out.mask.push_back(true);
        }
    }
    return out;
}

Address find_pattern(Address base, std::size_t size, const Pattern& pat) {
    if (pat.bytes.empty() || size < pat.bytes.size()) return 0;
    const auto* start = reinterpret_cast<const std::uint8_t*>(base);
    const std::size_t scan_end = size - pat.bytes.size();
    for (std::size_t off = 0; off <= scan_end; ++off) {
        bool ok = true;
        for (std::size_t j = 0; j < pat.bytes.size(); ++j) {
            if (pat.mask[j] && start[off + j] != pat.bytes[j]) { ok = false; break; }
        }
        if (ok) return base + off;
    }
    return 0;
}

std::vector<Address> find_lea_refs_to(Address text_base, std::size_t text_size, Address target_address) {
    // x86-64 LEA reg, [rip+disp32] is 7 bytes:
    //   48 8D xx ?? ?? ?? ??
    // (REX.W=1, opcode 8D, ModRM.mod=00, ModRM.rm=101 -> RIP-relative).
    // The 4-byte disp is signed; the operand resolves at the byte AFTER the
    // disp (i.e. instruction_addr + 7).
    std::vector<Address> hits;
    if (text_size < 7) return hits;
    const auto* p = reinterpret_cast<const std::uint8_t*>(text_base);
    const std::size_t end = text_size - 7;
    for (std::size_t off = 0; off <= end; ++off) {
        if (p[off] != 0x48 || p[off + 1] != 0x8D) continue;
        // ModRM: mod=00 (high 2 bits = 0), rm=101 (low 3 bits = 5).
        const std::uint8_t modrm = p[off + 2];
        if ((modrm & 0xC7) != 0x05) continue;
        std::int32_t disp;
        std::memcpy(&disp, p + off + 3, sizeof(disp));
        const Address next_ip = text_base + off + 7;
        const Address operand = next_ip + static_cast<std::int64_t>(disp);
        if (operand == target_address) hits.push_back(text_base + off);
    }
    return hits;
}

} // namespace gbfr::core
