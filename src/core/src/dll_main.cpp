// SPDX-License-Identifier: MIT
// dll_main.cpp — Windows entry point.
//
// On `DLL_PROCESS_ATTACH` the DLL auto-initializes the default session in
// internal mode. If the host already injected before the game module was
// loaded this is a no-op (attach returns GBFR_ERR_NOT_FOUND), and the host
// is free to call `gbfr_init(GBFR_ATTACH_INTERNAL, NULL)` later.

#if defined(_WIN32)

#include "gbfr/core/c_api.h"

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE /*inst*/, DWORD reason, LPVOID /*reserved*/) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            // Best-effort auto-attach. Ignore failure (e.g. when the DLL is
            // loaded into a process other than the game during testing).
            (void)gbfr_init(GBFR_ATTACH_INTERNAL, nullptr);
            break;
        case DLL_PROCESS_DETACH:
            gbfr_shutdown();
            break;
        default:
            break;
    }
    return TRUE;
}

#endif // _WIN32
