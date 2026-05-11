// SPDX-License-Identifier: MIT
// inject.cpp — CreateRemoteThread + LoadLibraryW injector.

#include "inject.hpp"

#include <windows.h>
#include <tlhelp32.h>

#include <cwchar>

namespace gbfr::app {

namespace {

DWORD find_pid_by_name(const std::wstring& exe_name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    DWORD found = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exe_name.c_str()) == 0) {
                found = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

} // namespace

const wchar_t* describe(InjectStatus s) {
    switch (s) {
    case InjectStatus::Ok:                  return L"ok";
    case InjectStatus::ProcessNotFound:     return L"target process not running";
    case InjectStatus::OpenProcessFailed:   return L"OpenProcess failed (try running as Administrator)";
    case InjectStatus::AllocFailed:         return L"VirtualAllocEx failed";
    case InjectStatus::WriteFailed:         return L"WriteProcessMemory failed";
    case InjectStatus::CreateThreadFailed:  return L"CreateRemoteThread failed";
    case InjectStatus::LoadLibraryFailed:   return L"LoadLibraryW in target returned 0";
    case InjectStatus::DllNotFound:         return L"DLL not found at given path";
    }
    return L"unknown";
}

InjectResult inject_dll(const std::wstring& dll_path, const std::wstring& target_exe) {
    InjectResult r;

    // Check the DLL exists locally before bothering the target.
    if (GetFileAttributesW(dll_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        r.status = InjectStatus::DllNotFound;
        r.detail = dll_path;
        return r;
    }

    const DWORD pid = find_pid_by_name(target_exe);
    if (pid == 0) {
        r.status = InjectStatus::ProcessNotFound;
        r.detail = target_exe;
        return r;
    }
    r.pid = pid;

    const DWORD access = PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION
                       | PROCESS_VM_WRITE | PROCESS_VM_READ
                       | PROCESS_QUERY_INFORMATION;
    HANDLE proc = OpenProcess(access, FALSE, pid);
    if (!proc) {
        r.status = InjectStatus::OpenProcessFailed;
        return r;
    }

    const SIZE_T bytes = (dll_path.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) {
        CloseHandle(proc);
        r.status = InjectStatus::AllocFailed;
        return r;
    }

    SIZE_T written = 0;
    if (!WriteProcessMemory(proc, remote, dll_path.c_str(), bytes, &written) || written != bytes) {
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        r.status = InjectStatus::WriteFailed;
        return r;
    }

    // kernel32.dll is loaded at the same address in every process on the same
    // boot, so the local LoadLibraryW pointer is valid in the remote process.
    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    auto load_library_w = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(k32, "LoadLibraryW"));
    if (!load_library_w) {
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        r.status = InjectStatus::CreateThreadFailed;
        return r;
    }

    HANDLE thr = CreateRemoteThread(proc, nullptr, 0, load_library_w, remote, 0, nullptr);
    if (!thr) {
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        r.status = InjectStatus::CreateThreadFailed;
        return r;
    }

    WaitForSingleObject(thr, INFINITE);

    DWORD exit_code = 0;
    GetExitCodeThread(thr, &exit_code);
    CloseHandle(thr);
    VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
    CloseHandle(proc);

    if (exit_code == 0) {
        r.status = InjectStatus::LoadLibraryFailed;
        return r;
    }

    r.status = InjectStatus::Ok;
    return r;
}

} // namespace gbfr::app
