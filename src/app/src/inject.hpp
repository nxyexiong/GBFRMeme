// SPDX-License-Identifier: MIT
// inject.hpp — minimal CreateRemoteThread + LoadLibraryW injector.
#pragma once

#include <string>

namespace gbfr::app {

enum class InjectStatus {
    Ok,
    ProcessNotFound,
    OpenProcessFailed,
    AllocFailed,
    WriteFailed,
    CreateThreadFailed,
    LoadLibraryFailed,
    DllNotFound,
};

struct InjectResult {
    InjectStatus status = InjectStatus::Ok;
    unsigned long pid   = 0;
    std::wstring  detail;
};

// Locate the running `granblue_fantasy_relink.exe` process, then inject the
// DLL at `dll_path` via VirtualAllocEx + WriteProcessMemory +
// CreateRemoteThread(LoadLibraryW).
InjectResult inject_dll(const std::wstring& dll_path,
                        const std::wstring& target_exe = L"granblue_fantasy_relink.exe");

// Human-readable name for an InjectStatus.
const wchar_t* describe(InjectStatus s);

} // namespace gbfr::app
