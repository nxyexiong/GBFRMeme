add_library(gbfr_sdk STATIC
    src/game.cpp
    src/item_types.cpp
    src/skill_types.cpp
)
target_include_directories(gbfr_sdk PUBLIC include)
target_compile_features(gbfr_sdk PUBLIC cxx_std_20)
set_target_properties(gbfr_sdk PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(gbfr::sdk ALIAS gbfr_sdk)
