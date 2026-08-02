// SPDX-License-Identifier: MIT
// main.cpp — GBFRMeme.exe entry point.
//
// Modes:
//   -h                       print help
//   -i                       inject gbfr_core.dll into the running game, exit
//   -c <subcmd> [args...]    CLI form (no GUI), see below
//   (no flag)                external form: gbfr_init(EXTERNAL) + GUI
//
// CLI subcommands (all run with GBFR_INIT_NO_UI):
//   ping                     attach, report success, detach
//   characters [name]        list characters, or look one up by name id
//   combat                   print the locally-controlled character's state

#include "gbfr/core/c_api.h"
#include "cli_debug.hpp"
#include "inject.hpp"

#include <windows.h>
#include <shellapi.h>
#include <fcntl.h>
#include <io.h>

#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

namespace {

enum class Mode { External, Inject, Cli, Help };

struct ParsedArgs {
    Mode                       mode = Mode::External;
    bool                       unknown = false;
    std::vector<std::wstring>  rest;     // remaining args for the chosen mode
};

std::string to_utf8(const std::wstring& w) {
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                                      nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                        out.data(), n, nullptr, nullptr);
    return out;
}

ParsedArgs parse_argv(int argc, wchar_t** argv) {
    ParsedArgs a;
    int i = 1;
    while (i < argc) {
        const std::wstring v = argv[i];
        if (v == L"-h" || v == L"--help" || v == L"/?") {
            a.mode = Mode::Help;
            return a;
        }
        if (v == L"-i" || v == L"--inject") {
            a.mode = Mode::Inject;
            ++i;
            continue;
        }
        if (v == L"-c" || v == L"--cli") {
            a.mode = Mode::Cli;
            for (int j = i + 1; j < argc; ++j) a.rest.emplace_back(argv[j]);
            return a;
        }
        a.unknown = true;
        return a;
    }
    return a;
}

void print_usage() {
    std::wprintf(
        L"GBFRMeme — Granblue Fantasy: Relink toolkit\n"
        L"\n"
        L"Usage:\n"
        L"  GBFRMeme.exe [-h]\n"
        L"  GBFRMeme.exe -i\n"
        L"  GBFRMeme.exe -c <subcommand> [args...]\n"
        L"  GBFRMeme.exe\n"
        L"\n"
        L"Modes:\n"
        L"  -h, --help                    Show this help and exit.\n"
        L"  -i, --inject                  Inject gbfr_core.dll into the running\n"
        L"                                game and exit.\n"
        L"  -c, --cli <subcmd>            CLI form (no GUI):\n"
        L"      ping                         Verify attach to the running game.\n"
        L"      characters [name_id]         List all characters, or look up one.\n"
        L"      combat                       Show the locally-controlled\n"
        L"                                   character's live state.\n"
        L"      items                        List inventory items.\n"
        L"      sigils                       List sigils (rolled instances).\n"
        L"      wrightstones                 List wrightstones (rolled).\n"
        L"      summons                      List owned summons.\n"
        L"      currency                     Show rupies + mastery points.\n"
        L"      debug <subcmd>               Low-level RE helpers; pass 'help'.\n"
        L"  (no flag)                     External form. Opens the GUI window\n"
        L"                                and attaches to the running game.\n"
        L"\n");
}

const wchar_t* status_name(GbfrStatus s) {
    switch (s) {
    case GBFR_OK:                 return L"OK";
    case GBFR_ERR_INVALID_ARG:    return L"INVALID_ARG";
    case GBFR_ERR_NOT_FOUND:      return L"NOT_FOUND";
    case GBFR_ERR_ALREADY_INIT:   return L"ALREADY_INIT";
    case GBFR_ERR_NOT_INIT:       return L"NOT_INIT";
    case GBFR_ERR_NOT_AVAILABLE:  return L"NOT_AVAILABLE";
    case GBFR_ERR_PLATFORM:       return L"PLATFORM";
    case GBFR_ERR_OUT_OF_RANGE:   return L"OUT_OF_RANGE";
    }
    return L"?";
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

// ---------------------------------------------------------------------------
// Modes
// ---------------------------------------------------------------------------

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
    // Hide the console window for external GUI mode. If launched from an
    // existing shell, this detaches the EXE from it; if launched from
    // Explorer (own console), this hides that console outright.
    if (HWND con = GetConsoleWindow()) {
        ShowWindow(con, SW_HIDE);
    }
    FreeConsole();

    GbfrStatus s = gbfr_init(GBFR_ATTACH_EXTERNAL, nullptr, GBFR_INIT_NONE);
    if (s != GBFR_OK) {
        return 4;
    }
    gbfr_wait_for_exit();
    gbfr_shutdown();
    return 0;
}

// ---- CLI subcommands -------------------------------------------------------

struct CliSession {
    GbfrStatus status = GBFR_ERR_NOT_INIT;
    CliSession()  { status = gbfr_init(GBFR_ATTACH_EXTERNAL, nullptr, GBFR_INIT_NO_UI); }
    ~CliSession() { if (status == GBFR_OK) gbfr_shutdown(); }
    explicit operator bool() const { return status == GBFR_OK; }
};

void print_character(const GbfrCharacterInfo& c) {
    std::wprintf(L"  name_id      : %hs\n", c.name_id);
    std::wprintf(L"  display_name : %hs\n", c.display_name);
    std::wprintf(L"  level        : %u\n", c.level);
    std::wprintf(L"  exp          : %llu\n", static_cast<unsigned long long>(c.exp));
    std::wprintf(L"  costume_id   : %hs\n", c.costume_id);
    std::wprintf(L"  color_index  : %u\n", c.color_index);
}

int cli_ping() {
    CliSession s;
    if (!s) {
        std::fwprintf(stderr, L"attach failed: %s\n", status_name(s.status));
        return 1;
    }
    std::wprintf(L"OK — attached to granblue_fantasy_relink.exe\n");
    return 0;
}

int cli_characters(const std::vector<std::wstring>& args) {
    CliSession s;
    if (!s) {
        std::fwprintf(stderr, L"attach failed: %s\n", status_name(s.status));
        return 1;
    }

    if (args.size() > 1) {
        std::fwprintf(stderr, L"too many arguments to 'characters'\n");
        return 1;
    }

    if (args.size() == 1) {
        const std::string narrow = to_utf8(args[0]);
        GbfrCharacterInfo info{};
        GbfrStatus st = gbfr_character_get_by_name(narrow.c_str(), &info);
        if (st != GBFR_OK) {
            std::wprintf(L"character '%hs': %s\n", narrow.c_str(), status_name(st));
            return (st == GBFR_ERR_NOT_AVAILABLE) ? 0 : 1;
        }
        std::wprintf(L"character '%hs':\n", narrow.c_str());
        print_character(info);
        return 0;
    }

    uint32_t count = 0;
    GbfrStatus st = gbfr_character_get_count(&count);
    if (st != GBFR_OK) {
        std::wprintf(L"character count: %s\n", status_name(st));
        return (st == GBFR_ERR_NOT_AVAILABLE) ? 0 : 1;
    }

    std::wprintf(L"%u character(s) in save:\n", count);
    for (uint32_t i = 0; i < count; ++i) {
        GbfrCharacterInfo info{};
        st = gbfr_character_get_at(i, &info);
        if (st != GBFR_OK) {
            std::wprintf(L"[%u]: %s\n", i, status_name(st));
            continue;
        }
        std::wprintf(L"[%u]\n", i);
        print_character(info);
    }
    return 0;
}

int cli_combat() {
    CliSession s;
    if (!s) {
        std::fwprintf(stderr, L"attach failed: %s\n", status_name(s.status));
        return 1;
    }
    GbfrCombatInfo info{};
    GbfrStatus st = gbfr_combat_get_current_info(&info);
    if (st != GBFR_OK) {
        std::wprintf(L"combat: %s\n", status_name(st));
        return (st == GBFR_ERR_NOT_AVAILABLE) ? 0 : 1;
    }

    std::wprintf(L"player_name             : %hs\n", info.player_name);
    std::wprintf(L"character_name_id       : %hs\n", info.character_name_id);
    std::wprintf(L"character_display_name  : %hs\n", info.character_display_name);
    std::wprintf(L"level                   : %u\n", info.level);
    std::wprintf(L"hp / hp_max             : %.1f / %.1f\n", info.hp, info.hp_max);
    std::wprintf(L"critical_rate           : %.3f\n", info.critical_rate);
    return 0;
}

int cli_items() {
    CliSession s;
    if (!s) {
        std::fwprintf(stderr, L"attach failed: %s\n", status_name(s.status));
        return 1;
    }
    uint32_t count = 0;
    GbfrStatus st = gbfr_item_get_count(&count);
    if (st != GBFR_OK) {
        std::wprintf(L"items: %s\n", status_name(st));
        return (st == GBFR_ERR_NOT_AVAILABLE) ? 0 : 1;
    }
    std::wprintf(L"%u item(s):\n", count);
    for (uint32_t i = 0; i < count; ++i) {
        GbfrItemInfo info{};
        st = gbfr_item_get_at(i, &info);
        if (st != GBFR_OK) {
            std::wprintf(L"[%u]: %s\n", i, status_name(st));
            continue;
        }
        std::wprintf(L"[%3u] %4u x %-32hs  (0x%08x %hs)\n",
                     i, info.count,
                     info.display_name[0] ? info.display_name : "<unknown>",
                     info.item_hash, info.asset_id);
    }
    return 0;
}

int cli_sigils() {
    CliSession s;
    if (!s) {
        std::fwprintf(stderr, L"attach failed: %s\n", status_name(s.status));
        return 1;
    }
    uint32_t count = 0;
    GbfrStatus st = gbfr_sigil_get_count(&count);
    if (st != GBFR_OK) {
        std::wprintf(L"sigils: %s\n", status_name(st));
        return (st == GBFR_ERR_NOT_AVAILABLE) ? 0 : 1;
    }
    std::wprintf(L"%u sigil(s):\n", count);
    for (uint32_t i = 0; i < count; ++i) {
        GbfrSigilInfo info{};
        st = gbfr_sigil_get_at(i, &info);
        if (st != GBFR_OK) {
            std::wprintf(L"[%u]: %s\n", i, status_name(st));
            continue;
        }
        const char* eq = info.equipped_character_id[0]
                           ? info.equipped_character_id : "-";
        std::wprintf(
            L"[%4u] %-28hs L%-2u | %-30hs L%-2u + %-30hs L%-2u | %hs\n",
            i,
            info.sigil_name[0] ? info.sigil_name : "<unknown>",
            info.sigil_level,
            info.first_trait_name[0] ? info.first_trait_name : "<unknown>",
            info.first_trait_level,
            info.second_trait_name[0] ? info.second_trait_name : "<unknown>",
            info.second_trait_level,
            eq);
    }
    return 0;
}

int cli_wrightstones() {
    CliSession s;
    if (!s) {
        std::fwprintf(stderr, L"attach failed: %s\n", status_name(s.status));
        return 1;
    }
    uint32_t count = 0;
    GbfrStatus st = gbfr_wrightstone_get_count(&count);
    if (st != GBFR_OK) {
        std::wprintf(L"wrightstones: %s\n", status_name(st));
        return (st == GBFR_ERR_NOT_AVAILABLE) ? 0 : 1;
    }
    std::wprintf(L"%u wrightstone(s):\n", count);
    for (uint32_t i = 0; i < count; ++i) {
        GbfrWrightstoneInfo info{};
        st = gbfr_wrightstone_get_at(i, &info);
        if (st != GBFR_OK) {
            std::wprintf(L"[%u]: %s\n", i, status_name(st));
            continue;
        }
        std::wprintf(
            L"[%4u] %-32hs | %-24hs L%-2u + %-24hs L%-2u + %-24hs L%-2u\n",
            i,
            info.template_name[0] ? info.template_name : "<unknown>",
            info.trait1_name[0] ? info.trait1_name : "<unknown>",
            info.trait1_level,
            info.trait2_name[0] ? info.trait2_name : "<unknown>",
            info.trait2_level,
            info.trait3_name[0] ? info.trait3_name : "<unknown>",
            info.trait3_level);
    }
    return 0;
}

int cli_summons() {
    CliSession session;
    if (!session) {
        std::fwprintf(stderr, L"attach failed: %s\n", status_name(session.status));
        return 1;
    }
    uint32_t count = 0;
    GbfrStatus st = gbfr_summon_get_count(&count);
    if (st != GBFR_OK) {
        std::wprintf(L"summons: %s\n", status_name(st));
        return (st == GBFR_ERR_NOT_AVAILABLE) ? 0 : 1;
    }
    std::wprintf(L"%u summon(s):\n", count);
    for (uint32_t i = 0; i < count; ++i) {
        GbfrSummonInfo info{};
        st = gbfr_summon_get_at(i, &info);
        if (st != GBFR_OK) {
            std::wprintf(L"[%u]: %s\n", i, status_name(st));
            continue;
        }
        std::wprintf(
            L"[%3u] %-28hs | %-24hs L%-2u | %-24hs %u%hs\n",
            i,
            info.summon_name[0] ? info.summon_name : "<unknown>",
            info.trait_name[0] ? info.trait_name : "<unknown>",
            info.trait_level,
            info.stat_type_name[0] ? info.stat_type_name : "<unknown>",
            info.stat_value,
            info.stat_is_percent ? "%" : "");
    }
    return 0;
}

int cli_currency() {
    CliSession s;
    if (!s) {
        std::fwprintf(stderr, L"attach failed: %s\n", status_name(s.status));
        return 1;
    }
    GbfrCurrencyInfo info{};
    GbfrStatus st = gbfr_currency_get_info(&info);
    if (st != GBFR_OK) {
        std::wprintf(L"currency: %s\n", status_name(st));
        return 1;
    }
    std::wprintf(L"wallet @ 0x%016llX\n", (unsigned long long)info.wallet_address);
    std::wprintf(L"  rupies         : %u\n", info.rupies);
    std::wprintf(L"  mastery_points : %u\n", info.mastery_points);
    return 0;
}

int run_cli(const std::vector<std::wstring>& args) {
    if (args.empty()) {
        std::fwprintf(stderr, L"missing CLI subcommand. Try: -c ping\n");
        return 1;
    }
    const std::wstring& cmd = args[0];
    std::vector<std::wstring> rest(args.begin() + 1, args.end());

    if (cmd == L"ping")        return cli_ping();
    if (cmd == L"characters")  return cli_characters(rest);
    if (cmd == L"combat")      return cli_combat();
    if (cmd == L"items")       return cli_items();
    if (cmd == L"sigils")      return cli_sigils();
    if (cmd == L"wrightstones") return cli_wrightstones();
    if (cmd == L"summons")      return cli_summons();
    if (cmd == L"currency")    return cli_currency();
    if (cmd == L"debug")       return gbfr::app::cli_debug(rest);

    std::fwprintf(stderr, L"unknown CLI subcommand: %s\n", cmd.c_str());
    return 1;
}

} // namespace

int main() {
    // Force stdout/stderr into UTF-16 mode so `wprintf` emits non-ASCII
    // wide chars (em-dashes, CJK, etc.) without the default C locale
    // truncating output at the first multibyte-conversion failure.
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    int       argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    auto args = parse_argv(argc, argv);
    if (argv) LocalFree(argv);

    if (args.unknown) { print_usage(); return 1; }
    switch (args.mode) {
    case Mode::Help:     print_usage();              return 0;
    case Mode::Inject:   return run_inject();
    case Mode::Cli:      return run_cli(args.rest);
    case Mode::External: return run_external();
    }
    return 0;
}
