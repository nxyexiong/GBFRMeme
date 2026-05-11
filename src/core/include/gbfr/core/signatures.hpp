// SPDX-License-Identifier: MIT
// gbfr/core/signatures.hpp — byte-pattern scanner for resolving runtime
// addresses that the SDK only knows by RVA or by neighbouring vftable.
#pragma once

#include "gbfr/common.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

namespace gbfr::core {

// Compiled pattern: bytes + per-byte mask (true = match exactly).
struct Pattern {
    std::vector<std::uint8_t> bytes;
    std::vector<bool>         mask;
};

// Parse an IDA-style pattern string ("48 8B ?? E8 ?? ?? ?? ??") into a
// Pattern. Empty input -> empty pattern. Whitespace separated tokens; each
// token is either two hex digits or "??".
[[nodiscard]] Pattern compile_pattern(std::string_view text);

// Linear scan inside [base, base+size). Returns the first match address or
// 0 if none.
[[nodiscard]] Address find_pattern(Address base, std::size_t size, const Pattern& pat);

// Walk all references to `target_rva` (e.g. a vftable RVA from the SDK) in
// the .text section, find ones that load it into rax/rcx with a `lea`
// instruction, then identify the function those `lea`s sit in. This is the
// recipe used by `cyan::Singleton<T>::installInstance(this)` callers, which
// is where each manager singleton stores its `this` pointer into a static
// global.
//
// NOTE: heuristic; the caller should verify the returned function.
[[nodiscard]] std::vector<Address> find_lea_refs_to(
    Address text_base, std::size_t text_size, Address target_address);

} // namespace gbfr::core
