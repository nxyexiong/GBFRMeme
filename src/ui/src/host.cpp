// SPDX-License-Identifier: MIT
// src/host.cpp — Win32 + D3D11 host for the ImGui-rendered UI.

#include "gbfr/ui/host.hpp"
#include "gbfr/ui/ui.hpp"

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <atomic>
#include <cstdint>
#include <utility>

// ImGui's Win32 backend exposes this for window procs to forward to.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace gbfr::ui {

namespace {

inline constexpr int kHotkeyId = 1;

template <class T>
void safe_release(T*& p) {
    if (p) { p->Release(); p = nullptr; }
}

} // namespace

struct Host::Impl {
    HostOptions   opts;
    DrawCallback  draw;

    HWND          hwnd       = nullptr;
    ATOM          wc_atom    = 0;
    DWORD         thread_id  = 0;
    HINSTANCE     hinstance  = nullptr;
    bool          visible    = true;

    ID3D11Device*           device     = nullptr;
    ID3D11DeviceContext*    context    = nullptr;
    IDXGISwapChain*         swapchain  = nullptr;
    ID3D11RenderTargetView* rtv        = nullptr;

    bool          imgui_initialized = false;
    bool          want_resize       = false;
    UINT          pending_width     = 0;
    UINT          pending_height    = 0;

    std::atomic<bool> stop_requested{false};

    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT wnd_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

    bool create_window();
    bool create_d3d();
    void create_rtv();
    void destroy_rtv();
    void handle_resize();
    void render_frame();
    void teardown();

    Impl(const HostOptions& o, DrawCallback d)
        : opts(o), draw(std::move(d)) {}
    ~Impl() { teardown(); }
};

// ---------------------------------------------------------------------------
// Window class / proc
// ---------------------------------------------------------------------------

LRESULT CALLBACK Host::Impl::WndProcStatic(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        const auto* cs = reinterpret_cast<const CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }
    auto* self = reinterpret_cast<Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->wnd_proc(hwnd, msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT Host::Impl::wnd_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (imgui_initialized) {
        if (ImGui_ImplWin32_WndProcHandler(wnd, msg, wp, lp))
            return 1;
    }

    switch (msg) {
    case WM_SIZE:
        if (wp != SIZE_MINIMIZED) {
            pending_width  = LOWORD(lp);
            pending_height = HIWORD(lp);
            want_resize    = true;
        }
        return 0;

    case WM_SYSCOMMAND:
        // Disable Alt+Space menu and screensaver while focused.
        if ((wp & 0xfff0) == SC_KEYMENU) return 0;
        break;

    case WM_HOTKEY:
        if (wp == kHotkeyId) {
            visible = !visible;
            ShowWindow(wnd, visible ? SW_SHOW : SW_HIDE);
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(wnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(wnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------
// Window + D3D11 setup
// ---------------------------------------------------------------------------

bool Host::Impl::create_window() {
    hinstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = &Impl::WndProcStatic;
    wc.hInstance     = hinstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"GbfrUiHostWndClass";
    wc_atom = RegisterClassExW(&wc);
    if (!wc_atom) return false;

    DWORD style    = WS_OVERLAPPEDWINDOW;
    DWORD ex_style = 0;
    if (opts.topmost) ex_style |= WS_EX_TOPMOST;

    RECT rc{0, 0, opts.width, opts.height};
    AdjustWindowRectEx(&rc, style, FALSE, ex_style);

    hwnd = CreateWindowExW(
        ex_style,
        MAKEINTATOM(wc_atom),
        opts.title.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hinstance, this);
    return hwnd != nullptr;
}

bool Host::Impl::create_d3d() {
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount                        = 2;
    scd.BufferDesc.Width                   = 0;
    scd.BufferDesc.Height                  = 0;
    scd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator   = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow                       = hwnd;
    scd.SampleDesc.Count                   = 1;
    scd.SampleDesc.Quality                 = 0;
    scd.Windowed                           = TRUE;
    scd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
    scd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL got{};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        flags, levels, _countof(levels), D3D11_SDK_VERSION,
        &scd, &swapchain, &device, &got, &context);
    if (FAILED(hr)) {
        // Try WARP as a last resort.
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            flags, levels, _countof(levels), D3D11_SDK_VERSION,
            &scd, &swapchain, &device, &got, &context);
        if (FAILED(hr)) return false;
    }

    create_rtv();
    return rtv != nullptr;
}

void Host::Impl::create_rtv() {
    ID3D11Texture2D* back = nullptr;
    if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&back))) || !back) return;
    device->CreateRenderTargetView(back, nullptr, &rtv);
    back->Release();
}

void Host::Impl::destroy_rtv() {
    safe_release(rtv);
}

void Host::Impl::handle_resize() {
    if (!want_resize || !swapchain) return;
    want_resize = false;
    destroy_rtv();
    swapchain->ResizeBuffers(0, pending_width, pending_height,
                             DXGI_FORMAT_UNKNOWN, 0);
    create_rtv();
}

void Host::Impl::render_frame() {
    if (!rtv) return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (draw) draw();

    ImGui::Render();

    constexpr float clear_color[4] = {0.10f, 0.10f, 0.12f, 1.0f};
    context->OMSetRenderTargets(1, &rtv, nullptr);
    context->ClearRenderTargetView(rtv, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    swapchain->Present(1, 0); // vsync on
}

void Host::Impl::teardown() {
    if (imgui_initialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imgui_initialized = false;
    }
    destroy_rtv();
    safe_release(swapchain);
    safe_release(context);
    safe_release(device);
    if (hwnd) {
        if (opts.hotkey_vk != 0) UnregisterHotKey(hwnd, kHotkeyId);
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }
    if (wc_atom && hinstance) {
        UnregisterClassW(MAKEINTATOM(wc_atom), hinstance);
        wc_atom = 0;
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

Host::Host(const HostOptions& opts, DrawCallback draw)
    : m_impl(std::make_unique<Impl>(opts, std::move(draw))) {}

Host::~Host() = default;

bool Host::run() {
    auto& s = *m_impl;
    s.thread_id = GetCurrentThreadId();

    if (!s.create_window()) return false;
    if (!s.create_d3d())    return false;

    if (s.opts.hotkey_vk != 0) {
        RegisterHotKey(s.hwnd, kHotkeyId,
                       s.opts.hotkey_mods,
                       static_cast<UINT>(s.opts.hotkey_vk));
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(s.hwnd);
    ImGui_ImplDX11_Init(s.device, s.context);
    s.imgui_initialized = true;

    s.visible = !s.opts.start_hidden;
    ShowWindow(s.hwnd, s.visible ? SW_SHOWDEFAULT : SW_HIDE);
    UpdateWindow(s.hwnd);

    MSG msg{};
    bool running = true;
    while (running) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
        }
        if (!running) break;

        if (s.stop_requested.load(std::memory_order_acquire)) {
            running = false;
            break;
        }

        s.handle_resize();

        if (s.visible && !IsIconic(s.hwnd)) {
            s.render_frame();
        } else {
            // Don't burn CPU while hidden / minimised.
            WaitMessage();
        }
    }

    s.teardown();
    return true;
}

void Host::request_stop() {
    if (!m_impl) return;
    m_impl->stop_requested.store(true, std::memory_order_release);
    if (m_impl->thread_id) {
        PostThreadMessageW(m_impl->thread_id, WM_NULL, 0, 0);
    }
}

} // namespace gbfr::ui
