// SPDX-License-Identifier: MIT
#include "gbfr/core/debug.hpp"

#include "gbfr/memory.hpp"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstring>

namespace gbfr::core::debug {

namespace {

constexpr std::size_t kChunkSize = 64 * 1024;

// Read a sliding window of the module in chunks; invoke `fn(buf_data,
// buf_len, file_offset_in_module)` for each chunk. `overlap` bytes of the
// previous chunk are retained so multi-byte patterns straddling boundaries
// are still found.
template <class Fn>
void walk_module(const Session& s, std::size_t overlap, Fn&& fn) {
    const Address base = module_base(s);
    const std::size_t size = module_size(s);
    if (size == 0) return;

    std::vector<std::uint8_t> buf(kChunkSize + overlap);
    for (std::size_t off = 0; off < size; off += kChunkSize) {
        const std::size_t want = std::min<std::size_t>(kChunkSize + overlap, size - off);
        if (!s.memory().read(base + off, buf.data(), want)) {
            continue;
        }
        if (!fn(buf.data(), want, off)) return;
    }
}

bool parse_pat_token(std::string_view tok, std::uint8_t& byte_out, bool& any_out) {
    if (tok == "??" || tok == "?") {
        any_out = true;
        byte_out = 0;
        return true;
    }
    if (tok.size() != 2) return false;
    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    };
    const int hi = hex(tok[0]);
    const int lo = hex(tok[1]);
    if (hi < 0 || lo < 0) return false;
    byte_out = static_cast<std::uint8_t>((hi << 4) | lo);
    any_out = false;
    return true;
}

struct Compiled {
    std::vector<std::uint8_t> bytes;
    std::vector<bool>         mask;   // true = match literal, false = wildcard
    bool ok = true;
};

Compiled compile_pattern(std::string_view text) {
    Compiled c;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
        if (i >= text.size()) break;
        std::size_t j = i;
        while (j < text.size() && !std::isspace(static_cast<unsigned char>(text[j]))) ++j;
        std::uint8_t b = 0; bool any = false;
        if (!parse_pat_token(text.substr(i, j - i), b, any)) {
            c.ok = false;
            return c;
        }
        c.bytes.push_back(b);
        c.mask.push_back(!any);
        i = j;
    }
    if (c.bytes.empty()) c.ok = false;
    return c;
}

} // namespace

// ---------- Module info ----------------------------------------------------

Address     module_base(const Session& s) noexcept { return s.module_info().base; }
std::size_t module_size(const Session& s) noexcept { return s.module_info().size; }

Address rva_to_abs(const Session& s, Address rva) noexcept {
    return s.rva(rva);
}

// ---------- Primitive reads -----------------------------------------------

bool read(const Session& s, Address addr, void* out, std::size_t n) {
    return s.memory().read(addr, out, n);
}

std::string read_cstr(const Session& s, Address addr, std::size_t max_n) {
    std::string out;
    out.reserve(std::min<std::size_t>(max_n, 64));
    std::vector<std::uint8_t> buf(max_n);
    if (!s.memory().read(addr, buf.data(), buf.size())) return {};
    for (std::size_t i = 0; i < buf.size(); ++i) {
        if (buf[i] == 0) break;
        out.push_back(static_cast<char>(buf[i]));
    }
    return out;
}

// ---------- Scans ----------------------------------------------------------

Address scan_qword(const Session& s, std::uint64_t value) {
    Address found = 0;
    walk_module(s, /*overlap=*/7, [&](const std::uint8_t* data, std::size_t len,
                                      std::size_t off) {
        const std::size_t limit = (len >= 8) ? (len - 7) : 0;
        for (std::size_t i = 0; i < limit; i += 8) {
            std::uint64_t v = 0;
            std::memcpy(&v, data + i, sizeof(v));
            if (v == value) {
                found = module_base(s) + off + i;
                return false;
            }
        }
        return true;
    });
    return found;
}

std::vector<Address> scan_qword_all(const Session& s, std::uint64_t value,
                                    std::size_t max_results) {
    std::vector<Address> out;
    walk_module(s, /*overlap=*/7, [&](const std::uint8_t* data, std::size_t len,
                                      std::size_t off) {
        const std::size_t limit = (len >= 8) ? (len - 7) : 0;
        for (std::size_t i = 0; i < limit; i += 8) {
            std::uint64_t v = 0;
            std::memcpy(&v, data + i, sizeof(v));
            if (v == value) {
                out.push_back(module_base(s) + off + i);
                if (out.size() >= max_results) return false;
            }
        }
        return true;
    });
    return out;
}

std::vector<Address> scan_qword_in_process(const Session& s, std::uint64_t value,
                                           std::size_t max_results) {
    std::vector<Address> out;
    HANDLE proc = static_cast<HANDLE>(s.process_handle());
    if (!proc) return out;

    // System address-range limits. Stop at the typical user-mode upper bound
    // for x64 (0x7FFF'FFFF'FFFF). Going higher walks into kernel addresses
    // VirtualQueryEx will refuse anyway.
    constexpr std::uint64_t kAddrMax = 0x7FFFFFFFFFFFULL;
    constexpr std::size_t   kChunk   = 1 << 20; // 1 MiB
    std::vector<std::uint8_t> buf;

    std::uint64_t cursor = 0;
    while (cursor < kAddrMax) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQueryEx(proc, reinterpret_cast<LPCVOID>(cursor),
                           &mbi, sizeof(mbi)) == 0) {
            break;
        }
        const auto region_base = reinterpret_cast<std::uint64_t>(mbi.BaseAddress);
        const auto region_size = static_cast<std::uint64_t>(mbi.RegionSize);
        const std::uint64_t next = region_base + region_size;

        const bool readable =
            mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) == 0 &&
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE
                          | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE
                          | PAGE_WRITECOPY    | PAGE_EXECUTE_WRITECOPY)) != 0;

        if (readable) {
            std::uint64_t off = 0;
            while (off < region_size) {
                const std::size_t want = static_cast<std::size_t>(
                    std::min<std::uint64_t>(kChunk + 7, region_size - off));
                if (buf.size() < want) buf.resize(want);
                if (s.memory().read(static_cast<Address>(region_base + off),
                                    buf.data(), want)) {
                    const std::size_t limit = (want >= 8) ? (want - 7) : 0;
                    for (std::size_t i = 0; i < limit; i += 8) {
                        std::uint64_t v = 0;
                        std::memcpy(&v, buf.data() + i, sizeof(v));
                        if (v == value) {
                            out.push_back(static_cast<Address>(region_base + off + i));
                            if (out.size() >= max_results) return out;
                        }
                    }
                }
                if (region_size - off <= kChunk) break;
                off += kChunk;
            }
        }

        if (next <= cursor) break; // avoid infinite loop on degenerate regions
        cursor = next;
    }
    return out;
}

std::vector<Address> scan_dword_in_process(const Session& s, std::uint32_t value,
                                           std::size_t max_results) {
    std::vector<Address> out;
    HANDLE proc = static_cast<HANDLE>(s.process_handle());
    if (!proc) return out;
    constexpr std::uint64_t kAddrMax = 0x7FFFFFFFFFFFULL;
    constexpr std::size_t   kChunk   = 1 << 20;
    std::vector<std::uint8_t> buf;

    std::uint64_t cursor = 0;
    while (cursor < kAddrMax) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQueryEx(proc, reinterpret_cast<LPCVOID>(cursor),
                           &mbi, sizeof(mbi)) == 0) {
            break;
        }
        const auto region_base = reinterpret_cast<std::uint64_t>(mbi.BaseAddress);
        const auto region_size = static_cast<std::uint64_t>(mbi.RegionSize);
        const std::uint64_t next = region_base + region_size;

        const bool readable =
            mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) == 0 &&
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE
                          | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE
                          | PAGE_WRITECOPY    | PAGE_EXECUTE_WRITECOPY)) != 0;

        if (readable) {
            std::uint64_t off = 0;
            while (off < region_size) {
                const std::size_t want = static_cast<std::size_t>(
                    std::min<std::uint64_t>(kChunk + 3, region_size - off));
                if (buf.size() < want) buf.resize(want);
                if (s.memory().read(static_cast<Address>(region_base + off),
                                    buf.data(), want)) {
                    const std::size_t limit = (want >= 4) ? (want - 3) : 0;
                    for (std::size_t i = 0; i < limit; ++i) {
                        std::uint32_t v = 0;
                        std::memcpy(&v, buf.data() + i, sizeof(v));
                        if (v == value) {
                            out.push_back(static_cast<Address>(region_base + off + i));
                            if (out.size() >= max_results) return out;
                        }
                    }
                }
                if (region_size - off <= kChunk) break;
                off += kChunk;
            }
        }

        if (next <= cursor) break;
        cursor = next;
    }
    return out;
}

Address scan_pattern(const Session& s, std::string_view pattern) {
    Compiled pat = compile_pattern(pattern);
    if (!pat.ok) return 0;
    Address found = 0;
    walk_module(s, /*overlap=*/pat.bytes.size() - 1, [&](const std::uint8_t* data,
                                                         std::size_t len,
                                                         std::size_t off) {
        if (len < pat.bytes.size()) return true;
        const std::size_t limit = len - pat.bytes.size() + 1;
        for (std::size_t i = 0; i < limit; ++i) {
            bool match = true;
            for (std::size_t k = 0; k < pat.bytes.size(); ++k) {
                if (pat.mask[k] && data[i + k] != pat.bytes[k]) { match = false; break; }
            }
            if (match) {
                found = module_base(s) + off + i;
                return false;
            }
        }
        return true;
    });
    return found;
}

std::vector<Address> scan_pattern_all(const Session& s, std::string_view pattern,
                                      std::size_t max_results) {
    std::vector<Address> out;
    Compiled pat = compile_pattern(pattern);
    if (!pat.ok) return out;
    walk_module(s, /*overlap=*/pat.bytes.size() - 1, [&](const std::uint8_t* data,
                                                         std::size_t len,
                                                         std::size_t off) {
        if (len < pat.bytes.size()) return true;
        const std::size_t limit = len - pat.bytes.size() + 1;
        for (std::size_t i = 0; i < limit; ++i) {
            bool match = true;
            for (std::size_t k = 0; k < pat.bytes.size(); ++k) {
                if (pat.mask[k] && data[i + k] != pat.bytes[k]) { match = false; break; }
            }
            if (match) {
                out.push_back(module_base(s) + off + i);
                if (out.size() >= max_results) return false;
            }
        }
        return true;
    });
    return out;
}

std::vector<Address> find_lea_refs_to(const Session& s, Address target,
                                      std::size_t max_results) {
    // Common 64-bit LEA forms with a RIP-relative 32-bit displacement:
    //   48 8D ?? ?? ?? ?? ??       (REX.W + LEA r64, [rip+disp32], modR/M byte)
    // We don't decode ModR/M precisely; instead we scan for a REX (0x48 or
    // 0x4C) followed by 0x8D followed by a ModR/M byte whose mod=00 and
    // r/m=101 (RIP-relative) i.e. (modrm & 0xC7) == 0x05.
    std::vector<Address> out;
    walk_module(s, /*overlap=*/8, [&](const std::uint8_t* data, std::size_t len,
                                      std::size_t off) {
        if (len < 7) return true;
        const std::size_t limit = len - 7;
        for (std::size_t i = 0; i < limit; ++i) {
            const std::uint8_t rex = data[i];
            if (rex != 0x48 && rex != 0x4C) continue;
            if (data[i + 1] != 0x8D) continue;
            const std::uint8_t modrm = data[i + 2];
            if ((modrm & 0xC7) != 0x05) continue;
            std::int32_t disp = 0;
            std::memcpy(&disp, data + i + 3, sizeof(disp));
            const Address ip_after = module_base(s) + off + i + 7;
            const Address effective = static_cast<Address>(
                static_cast<std::int64_t>(ip_after) + disp);
            if (effective == target) {
                out.push_back(module_base(s) + off + i);
                if (out.size() >= max_results) return false;
            }
        }
        return true;
    });
    return out;
}

} // namespace gbfr::core::debug
