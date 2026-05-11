// SPDX-License-Identifier: MIT
#include "gbfr/core/c_api.h"

#include "gbfr/core/session.hpp"

#include <memory>
#include <mutex>

namespace {

std::mutex                              g_mutex;
std::unique_ptr<gbfr::core::Session>    g_session;

std::unique_ptr<gbfr::core::Session> make_session(GbfrAttachMode mode, const wchar_t* exe) {
    if (mode == GBFR_ATTACH_INTERNAL) {
        auto s = gbfr::core::Session::attach_in_process();
        return s.has_value()
            ? std::make_unique<gbfr::core::Session>(std::move(*s))
            : nullptr;
    }
    std::wstring name = (exe != nullptr) ? std::wstring{exe} : std::wstring{};
    auto s = name.empty()
        ? gbfr::core::Session::attach_external_by_name()
        : gbfr::core::Session::attach_external_by_name(name);
    return s.has_value()
        ? std::make_unique<gbfr::core::Session>(std::move(*s))
        : nullptr;
}

} // namespace

// ---------------------------------------------------------------------------
// 1. Lifecycle
// ---------------------------------------------------------------------------

extern "C" {

GBFR_CORE_API uint32_t GBFR_CORE_CALL gbfr_api_version(void) {
    return 1u;
}

GBFR_CORE_API GbfrStatus GBFR_CORE_CALL gbfr_init(GbfrAttachMode mode, const wchar_t* exe) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_session) return GBFR_ERR_ALREADY_INIT;
    auto s = make_session(mode, exe);
    if (!s) return GBFR_ERR_NOT_FOUND;
    g_session = std::move(s);
    return GBFR_OK;
}

GBFR_CORE_API void GBFR_CORE_CALL gbfr_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_session.reset();
}

GBFR_CORE_API int GBFR_CORE_CALL gbfr_is_initialized(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_session ? 1 : 0;
}

// ---------------------------------------------------------------------------
// 2..4. Manager sections — intentionally empty for now.
//        Each section will fill in once we decide on its surface.
// ---------------------------------------------------------------------------

} // extern "C"
