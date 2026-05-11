// SPDX-License-Identifier: MIT
// gbfr/core/process_attach.hpp — locate and read the running game module.
#pragma once

#include "gbfr/common.hpp"

#include <optional>
#include <string>

namespace gbfr::core {

// Result of a successful module lookup.
struct ModuleInfo {
    Address     base{0};   // load address (HMODULE) of granblue_fantasy_relink.exe
    std::size_t size{0};   // image size in bytes (SizeOfImage)
};

// Find the loaded module by name. Defaults to "granblue_fantasy_relink.exe".
// Returns std::nullopt if the module is not loaded in the current process.
[[nodiscard]] std::optional<ModuleInfo> find_loaded_module(
    const std::wstring& module_name = L"granblue_fantasy_relink.exe");

} // namespace gbfr::core
