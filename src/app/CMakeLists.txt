# GBFRMeme.exe — thin launcher / injector.
#
# Builds a tiny CLI that:
#   *  -h           prints usage
#   *  -i           injects gbfr_core.dll into the game and exits
#   *  -c           CLI form (placeholder for now)
#   *  (no args)    external form — calls gbfr_init(EXTERNAL) and blocks
#                   on gbfr_wait_for_exit()
#
# The app only consumes `gbfr_core`'s public C ABI; it does not link
# `gbfr_ui` or `gbfr_sdk` directly.

add_executable(gbfr_app
    src/main.cpp
    src/inject.cpp
)
set_target_properties(gbfr_app PROPERTIES
    OUTPUT_NAME "GBFRMeme"
)
target_link_libraries(gbfr_app PRIVATE gbfr::core)
target_compile_features(gbfr_app PRIVATE cxx_std_20)
target_include_directories(gbfr_app PRIVATE src)

# Place GBFRMeme.exe next to gbfr_core.dll so the EXE can locate the DLL
# relative to itself at runtime (used by the -i injection path).
add_custom_command(TARGET gbfr_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:gbfr_core>"
        "$<TARGET_FILE_DIR:gbfr_app>"
    VERBATIM
)
