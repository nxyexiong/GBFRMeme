// SPDX-License-Identifier: MIT
#include "gbfr/core/session.hpp"

#include "gbfr/core/memory/internal_memory.hpp"
#include "gbfr/core/memory/external_memory.hpp"

namespace gbfr::core {

Session::Session(ModuleInfo info, std::unique_ptr<gbfr::IMemory> mem) noexcept
    : m_module(info), m_memory(std::move(mem)),
      m_game(std::make_unique<gbfr::Game>(*m_memory)) {}

std::optional<Session> Session::attach_in_process() {
    auto info = find_loaded_module();
    if (!info.has_value()) return std::nullopt;
    auto mem = std::make_unique<InternalMemory>(info->base);
    return Session{*info, std::move(mem)};
}

std::optional<Session> Session::attach_external_by_name(const std::wstring& executable_name) {
    auto ext = ExternalMemory::attach_by_name(executable_name);
    if (ext == nullptr) return std::nullopt;
    ModuleInfo info{ext->module_base(), ext->module_size()};
    return Session{info, std::move(ext)};
}

std::size_t Session::resolve_singletons_via_name_strings() {
    // Each manager records `kNameStringRva` of its class-name string in
    // .rdata. The `cyan::Singleton<T>::registerInstance` accessor LEAs that
    // string into a register before calling the registry. By finding the
    // first `lea rcx,[name]` within the live module and walking forward
    // through the enclosing function we could locate the static slot.
    //
    // The exact disassembly walk requires a decoder (e.g. Zydis). For now
    // this method is a stub that records candidate LEA sites for the
    // host to verify with a debugger. Returns 0 until implemented.
    return 0;
}

Address Session::rva(Address rva_value) const noexcept {
    return rva_to_absolute(m_module.base, rva_value);
}

} // namespace gbfr::core
