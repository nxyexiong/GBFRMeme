// SPDX-License-Identifier: MIT
// src/overlay.cpp — IDXGISwapChain vtable hook + ImGui DX11 rendering.

#include "gbfr/ui/overlay.hpp"

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <atomic>
#include <mutex>
#include <utility>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace gbfr::ui {

namespace {

using PresentFn       = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT,
                                                    DXGI_FORMAT, UINT);

// IDXGISwapChain vtable indices (IUnknown=0..2, IDXGIObject=3..6,
// IDXGIDeviceSubObject=7, IDXGISwapChain Present=8, GetBuffer=9,
// SetFullscreenState=10, GetFullscreenState=11, GetDesc=12,
// ResizeBuffers=13, ...).
constexpr std::size_t kVtIdxPresent       = 8;
constexpr std::size_t kVtIdxResizeBuffers = 13;

// Hook + render state. There is at most one active overlay per process.
struct OverlayState {
    std::mutex            mutex;
    bool                  hooked         = false;
    void**                vtable         = nullptr;
    PresentFn             orig_present   = nullptr;
    ResizeBuffersFn       orig_resize    = nullptr;

    // First-time initialisation captures these from the game's swap chain.
    bool                     imgui_initialised = false;
    ID3D11Device*            device  = nullptr;
    ID3D11DeviceContext*     context = nullptr;
    ID3D11RenderTargetView*  rtv     = nullptr;
    HWND                     hwnd    = nullptr;
    WNDPROC                  orig_wndproc = nullptr;

    // Behaviour knobs.
    DrawCallback             draw;
    int                      hotkey_vk     = 0;
    bool                     hotkey_prev   = false;
    std::atomic<bool>        visible{false};
    std::atomic<bool>        uninstalling{false};
};

OverlayState g_state;

// ---------------------------------------------------------------------------
// Hooks
// ---------------------------------------------------------------------------

LRESULT CALLBACK overlay_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (g_state.imgui_initialised && !g_state.uninstalling.load()) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);

        if (g_state.visible.load()) {
            const ImGuiIO& io = ImGui::GetIO();
            switch (msg) {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
            case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
            case WM_MOUSEWHEEL:  case WM_MOUSEHWHEEL:
                if (io.WantCaptureMouse) return 0;
                break;
            case WM_KEYDOWN: case WM_KEYUP:
            case WM_SYSKEYDOWN: case WM_SYSKEYUP:
            case WM_CHAR:
                if (io.WantCaptureKeyboard) return 0;
                break;
            default:
                break;
            }
        }
    }
    return CallWindowProcW(g_state.orig_wndproc, hwnd, msg, wp, lp);
}

bool ensure_initialised(IDXGISwapChain* sc) {
    if (g_state.imgui_initialised) return true;

    ID3D11Device* dev = nullptr;
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&dev))) || !dev)
        return false;
    g_state.device = dev;
    dev->GetImmediateContext(&g_state.context);

    DXGI_SWAP_CHAIN_DESC desc{};
    sc->GetDesc(&desc);
    g_state.hwnd = desc.OutputWindow;

    ID3D11Texture2D* back = nullptr;
    if (SUCCEEDED(sc->GetBuffer(0, IID_PPV_ARGS(&back))) && back) {
        g_state.device->CreateRenderTargetView(back, nullptr, &g_state.rtv);
        back->Release();
    }
    if (!g_state.rtv || !g_state.hwnd) return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(g_state.hwnd);
    ImGui_ImplDX11_Init(g_state.device, g_state.context);

    g_state.orig_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        g_state.hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&overlay_wndproc)));

    g_state.imgui_initialised = true;
    return true;
}

void poll_hotkey() {
    if (g_state.hotkey_vk == 0) return;
    const bool down = (GetAsyncKeyState(g_state.hotkey_vk) & 0x8000) != 0;
    if (down && !g_state.hotkey_prev) {
        g_state.visible.store(!g_state.visible.load());
    }
    g_state.hotkey_prev = down;
}

HRESULT STDMETHODCALLTYPE Present_hook(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (g_state.uninstalling.load()) {
        return g_state.orig_present(sc, sync, flags);
    }

    if (ensure_initialised(sc)) {
        poll_hotkey();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g_state.visible.load() && g_state.draw) {
            g_state.draw();
        }

        ImGui::Render();
        if (g_state.rtv) {
            g_state.context->OMSetRenderTargets(1, &g_state.rtv, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }
    }

    return g_state.orig_present(sc, sync, flags);
}

HRESULT STDMETHODCALLTYPE ResizeBuffers_hook(IDXGISwapChain* sc, UINT count, UINT w, UINT h,
                                             DXGI_FORMAT fmt, UINT flags) {
    if (g_state.rtv) {
        g_state.rtv->Release();
        g_state.rtv = nullptr;
    }
    const HRESULT hr = g_state.orig_resize(sc, count, w, h, fmt, flags);
    if (SUCCEEDED(hr) && g_state.device) {
        ID3D11Texture2D* back = nullptr;
        if (SUCCEEDED(sc->GetBuffer(0, IID_PPV_ARGS(&back))) && back) {
            g_state.device->CreateRenderTargetView(back, nullptr, &g_state.rtv);
            back->Release();
        }
    }
    return hr;
}

// ---------------------------------------------------------------------------
// vtable patching
// ---------------------------------------------------------------------------

bool patch_vt_slot(void** vtable, std::size_t index, void* replacement, void*& out_original) {
    DWORD old_prot = 0;
    if (!VirtualProtect(&vtable[index], sizeof(void*), PAGE_READWRITE, &old_prot)) {
        return false;
    }
    out_original     = vtable[index];
    vtable[index]    = replacement;
    DWORD restored = 0;
    VirtualProtect(&vtable[index], sizeof(void*), old_prot, &restored);
    return true;
}

bool acquire_dummy_swapchain_vtable(void**& out_vtable) {
    HWND dummy = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPED,
                                 0, 0, 1, 1, nullptr, nullptr,
                                 GetModuleHandleW(nullptr), nullptr);
    if (!dummy) return false;

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount       = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow      = dummy;
    desc.SampleDesc.Count  = 1;
    desc.Windowed          = TRUE;
    desc.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL    got{};
    ID3D11Device*        dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    IDXGISwapChain*      sc  = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, _countof(levels), D3D11_SDK_VERSION,
        &desc, &sc, &dev, &got, &ctx);
    if (FAILED(hr) || !sc) {
        if (ctx) ctx->Release();
        if (dev) dev->Release();
        DestroyWindow(dummy);
        return false;
    }

    out_vtable = *reinterpret_cast<void***>(sc);

    sc->Release();
    if (ctx) ctx->Release();
    if (dev) dev->Release();
    DestroyWindow(dummy);
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

Overlay::Overlay(const OverlayOptions& opts, DrawCallback draw) {
    g_state.draw        = std::move(draw);
    g_state.hotkey_vk   = opts.hotkey_vk;
    g_state.visible.store(opts.start_visible);
}

Overlay::~Overlay() { uninstall(); }

bool Overlay::install() {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (g_state.hooked) return true;

    void** vtable = nullptr;
    if (!acquire_dummy_swapchain_vtable(vtable) || !vtable) {
        return false;
    }
    g_state.vtable = vtable;

    void* orig_present_raw = nullptr;
    void* orig_resize_raw  = nullptr;
    if (!patch_vt_slot(vtable, kVtIdxPresent,
                       reinterpret_cast<void*>(&Present_hook), orig_present_raw)) {
        return false;
    }
    if (!patch_vt_slot(vtable, kVtIdxResizeBuffers,
                       reinterpret_cast<void*>(&ResizeBuffers_hook), orig_resize_raw)) {
        // Try to restore the Present slot we already patched.
        void* dummy = nullptr;
        patch_vt_slot(vtable, kVtIdxPresent, orig_present_raw, dummy);
        return false;
    }

    g_state.orig_present = reinterpret_cast<PresentFn>(orig_present_raw);
    g_state.orig_resize  = reinterpret_cast<ResizeBuffersFn>(orig_resize_raw);
    g_state.hooked       = true;
    return true;
}

void Overlay::uninstall() {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!g_state.hooked) return;

    g_state.uninstalling.store(true);

    // Restore the vtable first so any in-flight Present that observes the
    // new values goes through the original path.
    if (g_state.vtable) {
        void* dummy = nullptr;
        if (g_state.orig_present) {
            patch_vt_slot(g_state.vtable, kVtIdxPresent,
                          reinterpret_cast<void*>(g_state.orig_present), dummy);
        }
        if (g_state.orig_resize) {
            patch_vt_slot(g_state.vtable, kVtIdxResizeBuffers,
                          reinterpret_cast<void*>(g_state.orig_resize), dummy);
        }
    }

    // Give any in-flight hooked call a moment to leave our code before we
    // tear down ImGui / D3D state. 100 ms is generous for a 60-Hz tick.
    Sleep(100);

    if (g_state.hwnd && g_state.orig_wndproc) {
        SetWindowLongPtrW(g_state.hwnd, GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(g_state.orig_wndproc));
    }

    if (g_state.imgui_initialised) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_state.imgui_initialised = false;
    }

    if (g_state.rtv)     { g_state.rtv->Release();     g_state.rtv     = nullptr; }
    if (g_state.context) { g_state.context->Release(); g_state.context = nullptr; }
    if (g_state.device)  { g_state.device->Release();  g_state.device  = nullptr; }

    g_state.hwnd          = nullptr;
    g_state.orig_wndproc  = nullptr;
    g_state.orig_present  = nullptr;
    g_state.orig_resize   = nullptr;
    g_state.vtable        = nullptr;
    g_state.hooked        = false;
    g_state.uninstalling.store(false);
}

} // namespace gbfr::ui
