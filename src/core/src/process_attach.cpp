// SPDX-License-Identifier: MIT
#include "gbfr/core/process_attach.hpp"

#include <windows.h>
#include <psapi.h>

namespace gbfr::core {

std::optional<ModuleInfo> find_loaded_module(const std::wstring& module_name) {
    HMODULE h = ::GetModuleHandleW(module_name.c_str());
    if (h == nullptr) return std::nullopt;

    MODULEINFO mi{};
    if (!::GetModuleInformation(::GetCurrentProcess(), h, &mi, sizeof(mi))) {
        return std::nullopt;
    }
    ModuleInfo out;
    out.base = reinterpret_cast<Address>(h);
    out.size = mi.SizeOfImage;
    return out;
}

} // namespace gbfr::core
