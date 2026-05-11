// SPDX-License-Identifier: MIT
#include "gbfr/core/memory/external_memory.hpp"

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <psapi.h>
    #include <tlhelp32.h>
    #include <wctype.h>
#endif

namespace gbfr::core {

#if defined(_WIN32)

namespace {

constexpr DWORD kDesiredAccess =
    PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION;

bool icase_equal(const wchar_t* a, const wchar_t* b) {
    if (a == nullptr || b == nullptr) return false;
    while (*a != L'\0' && *b != L'\0') {
        if (towlower(static_cast<wint_t>(*a)) != towlower(static_cast<wint_t>(*b))) return false;
        ++a; ++b;
    }
    return *a == L'\0' && *b == L'\0';
}

std::optional<std::pair<gbfr::Address, std::size_t>> find_module_in_handle(
    HANDLE process, const std::wstring& module_name) {
    HMODULE mods[1024];
    DWORD needed = 0;
    if (!EnumProcessModulesEx(process, mods, sizeof(mods), &needed, LIST_MODULES_ALL)) {
        return std::nullopt;
    }
    const std::size_t count = needed / sizeof(HMODULE);
    for (std::size_t i = 0; i < count; ++i) {
        wchar_t name[MAX_PATH];
        if (GetModuleBaseNameW(process, mods[i], name, MAX_PATH) == 0) continue;
        if (!icase_equal(name, module_name.c_str())) continue;
        MODULEINFO mi{};
        if (!GetModuleInformation(process, mods[i], &mi, sizeof(mi))) continue;
        return std::make_pair(reinterpret_cast<gbfr::Address>(mods[i]),
                              static_cast<std::size_t>(mi.SizeOfImage));
    }
    return std::nullopt;
}

DWORD find_pid_by_executable(const std::wstring& exe) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    DWORD found = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (icase_equal(pe.szExeFile, exe.c_str())) {
                found = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

} // namespace

std::unique_ptr<ExternalMemory> ExternalMemory::attach(std::uint32_t pid, const std::wstring& module_name) {
    HANDLE h = OpenProcess(kDesiredAccess, FALSE, pid);
    if (h == nullptr) return nullptr;
    auto mod = find_module_in_handle(h, module_name);
    if (!mod) {
        CloseHandle(h);
        return nullptr;
    }
    return std::unique_ptr<ExternalMemory>(new ExternalMemory(pid, h, mod->first, mod->second));
}

std::unique_ptr<ExternalMemory> ExternalMemory::attach_by_name(const std::wstring& executable_name) {
    DWORD pid = find_pid_by_executable(executable_name);
    if (pid == 0) return nullptr;
    return attach(static_cast<std::uint32_t>(pid), executable_name);
}

ExternalMemory::ExternalMemory(std::uint32_t pid, void* handle,
                               gbfr::Address module_base, std::size_t module_size) noexcept
    : m_pid(pid), m_handle(handle), m_module_base(module_base), m_module_size(module_size) {}

ExternalMemory::~ExternalMemory() {
    if (m_handle != nullptr) {
        CloseHandle(static_cast<HANDLE>(m_handle));
        m_handle = nullptr;
    }
}

bool ExternalMemory::read(gbfr::Address addr, void* out, std::size_t size) const {
    if (m_handle == nullptr || addr == 0 || out == nullptr) return false;
    SIZE_T got = 0;
    return ReadProcessMemory(static_cast<HANDLE>(m_handle),
                             reinterpret_cast<LPCVOID>(addr), out, size, &got) != 0
           && got == size;
}

bool ExternalMemory::write(gbfr::Address addr, const void* in, std::size_t size) {
    if (m_handle == nullptr || addr == 0 || in == nullptr) return false;
    SIZE_T put = 0;
    return WriteProcessMemory(static_cast<HANDLE>(m_handle),
                              reinterpret_cast<LPVOID>(addr), in, size, &put) != 0
           && put == size;
}

#else  // !_WIN32

std::unique_ptr<ExternalMemory> ExternalMemory::attach(std::uint32_t, const std::wstring&) {
    return nullptr;
}
std::unique_ptr<ExternalMemory> ExternalMemory::attach_by_name(const std::wstring&) {
    return nullptr;
}
ExternalMemory::ExternalMemory(std::uint32_t pid, void* handle,
                               gbfr::Address module_base, std::size_t module_size) noexcept
    : m_pid(pid), m_handle(handle), m_module_base(module_base), m_module_size(module_size) {}
ExternalMemory::~ExternalMemory() = default;
bool ExternalMemory::read(gbfr::Address, void*, std::size_t) const { return false; }
bool ExternalMemory::write(gbfr::Address, const void*, std::size_t) { return false; }

#endif // _WIN32

} // namespace gbfr::core
