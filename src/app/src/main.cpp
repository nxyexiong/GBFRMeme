// SPDX-License-Identifier: MIT
// main.cpp — GBFRMeme.exe entry point.
//
// Modes (mutually exclusive):
//   -h            print help and exit
//   -i            inject gbfr_core.dll into the running game, then exit
//   -c            CLI form (placeholder)
//   (no flag)     external form: gbfr_init(EXTERNAL) + gbfr_wait_for_exit()

#include "gbfr/core/c_api.h"
#include "inject.hpp"

#include <windows.h>
#include <shellapi.h>

#include <cstdio>
#include <cwchar>
#include <string>

namespace {

enum class Mode { External, Inject, Cli, Help };

struct ParsedArgs {
    Mode mode    = Mode::External;
    bool unknown = false;
};

ParsedArgs parse_argv(int argc, wchar_t** argv) {
    ParsedArgs a;
    for (int i = 1; i < argc; ++i) {
        const std::wstring v = argv[i];
        if (v == L"-h" || v == L"--help" || v == L"/?") {
            a.mode = Mode::Help;
            return a;
        }
        if (v == L"-i" || v == L"--inject") {
            a.mode = Mode::Inject;
        } else if (v == L"-c" || v == L"--cli") {
            a.mode = Mode::Cli;
        } else {
            a.unknown = true;
            return a;
        }
    }
    return a;
}

void print_usage() {
    std::wprintf(
        L"GBFRMeme — Granblue Fantasy: Relink toolkit\n"
        L"\n"
        L"Usage: GBFRMeme.exe [-h | -i | -c]\n"
        L"\n"
        L"  -h, --help    Show this help and exit.\n"
        L"  -i, --inject  Inject gbfr_core.dll into the running game and exit.\n"
        L"                The DLL must sit next to GBFRMeme.exe.\n"
        L"  -c, --cli     CLI form (not implemented yet).\n"
        L"  (no flag)     External form. Opens the GUI window and attaches to\n"
        L"                the running game via ReadProcessMemory.\n"
        L"\n");
}

std::wstring exe_dir() {
    wchar_t buf[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return {};
    std::wstring s(buf, n);
    const auto slash = s.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return {};
    return s.substr(0, slash);
}

int run_inject() {
    const std::wstring dir = exe_dir();
    if (dir.empty()) {
        std::fwprintf(stderr, L"could not resolve EXE directory\n");
        return 2;
    }
    const std::wstring dll = dir + L"\\gbfr_core.dll";

    std::wprintf(L"Injecting %s ...\n", dll.c_str());
    auto r = gbfr::app::inject_dll(dll);
    if (r.status != gbfr::app::InjectStatus::Ok) {
        std::fwprintf(stderr, L"inject failed: %s",
                      gbfr::app::describe(r.status));
        if (!r.detail.empty()) std::fwprintf(stderr, L" (%s)", r.detail.c_str());
        std::fwprintf(stderr, L"\n");
        return 3;
    }
    std::wprintf(L"Injected into PID %lu.\n", r.pid);
    return 0;
}

int run_external() {
    GbfrStatus s = gbfr_init(GBFR_ATTACH_EXTERNAL, nullptr);
    if (s != GBFR_OK) {
        std::fwprintf(stderr, L"gbfr_init failed: status=%d\n", static_cast<int>(s));
        return 4;
    }
    std::wprintf(L"External session running. Close the window to exit.\n");
    gbfr_wait_for_exit();
    gbfr_shutdown();
    return 0;
}

int run_cli() {
    std::wprintf(L"CLI form not yet implemented.\n");
    return 0;
}

} // namespace

int main() {
    int       argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    auto args = parse_argv(argc, argv);
    if (argv) LocalFree(argv);

    if (args.unknown) {
        print_usage();
        return 1;
    }
    switch (args.mode) {
    case Mode::Help:     print_usage();          return 0;
    case Mode::Inject:   return run_inject();
    case Mode::Cli:      return run_cli();
    case Mode::External: return run_external();
    }
    return 0;
}
