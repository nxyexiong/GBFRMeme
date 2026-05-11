# gbfr_ui — ImGui-based UI hosted in a Win32 + D3D11 window.
#
# This target also defines the `imgui` static library that bundles upstream
# ImGui sources from third_party/imgui/, with warnings dialled down (it does
# not build cleanly under /W4 /permissive-).

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")

if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR
        "ImGui sources not found at ${IMGUI_DIR}. "
        "Run: git submodule update --init --recursive")
endif()

# ---- imgui ----------------------------------------------------------------

add_library(imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp"
)
target_include_directories(imgui PUBLIC
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)
target_compile_features(imgui PUBLIC cxx_std_20)
if(MSVC)
    # Upstream ImGui doesn't build clean at /W4 /permissive-.
    target_compile_options(imgui PRIVATE /W0 /permissive)
endif()
target_link_libraries(imgui PUBLIC d3d11 dxgi d3dcompiler)

# ---- gbfr_ui --------------------------------------------------------------

add_library(gbfr_ui STATIC
    src/host.cpp
    src/overlay.cpp
    src/ui.cpp
)
target_include_directories(gbfr_ui PUBLIC include)
target_compile_features(gbfr_ui PUBLIC cxx_std_20)
target_link_libraries(gbfr_ui PUBLIC imgui)
set_target_properties(gbfr_ui PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(gbfr::ui ALIAS gbfr_ui)
