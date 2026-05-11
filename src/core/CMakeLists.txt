add_library(gbfr_core SHARED
    src/memory/internal_memory.cpp
    src/memory/external_memory.cpp
    src/process_attach.cpp
    src/signatures.cpp
    src/session.cpp
    src/ui_runtime.cpp
    src/c_api.cpp
    src/dll_main.cpp
)
target_include_directories(gbfr_core PUBLIC include)
target_link_libraries(gbfr_core PUBLIC gbfr::sdk psapi PRIVATE gbfr::ui)
target_compile_features(gbfr_core PUBLIC cxx_std_20)
target_compile_definitions(gbfr_core PRIVATE GBFR_CORE_BUILDING_DLL)
set_target_properties(gbfr_core PROPERTIES
    OUTPUT_NAME "gbfr_core"
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
)

add_library(gbfr::core ALIAS gbfr_core)
