// SPDX-License-Identifier: MIT
// cli_debug.hpp — CLI surface for the `gbfr_debug_*` C API.
//
// Subcommands (invoked via `GBFRMeme.exe -c debug <sub> [args]`):
//   module-info                     base + size of the attached module
//   rva <hex>                       Ghidra RVA -> live address
//   read-u32   <hex_addr>
//   read-u64   <hex_addr>
//   read-f32   <hex_addr>
//   read-bytes <hex_addr> <n>       hexdump of `n` bytes
//   read-cstr  <hex_addr> [max=256]
//   scan-qword <hex_value>          first occurrence
//   scan-qword-all <hex_value> [cap=16]
//   scan-pattern "<ida>"            first occurrence (quote the pattern)
//   scan-pattern-all "<ida>" [cap=16]
//   lea-refs   <hex_addr> [cap=16]  find LEAs whose effective addr == target
//
// Hex args may be written with or without a leading 0x.
#pragma once

#include <string>
#include <vector>

namespace gbfr::app {

int cli_debug(const std::vector<std::wstring>& args);

} // namespace gbfr::app
