// SPDX-License-Identifier: MIT
#include "gbfr/core/process_attach.hpp"

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <psapi.h>
#endif

namespace gbfr::core {

std::optional<ModuleInfo> find_loaded_module(const std::wstring& module_name) {
#if defined(_WIN32)
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
#else
    (void)module_name;
    return std::nullopt;
#endif
}

} // namespace gbfr::core
