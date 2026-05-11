cmake_minimum_required(VERSION 3.20)
project(GBFRMeme CXX)

if(NOT WIN32)
    message(FATAL_ERROR "GBFRMeme is Windows-only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8)
add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

add_subdirectory(src/sdk)
add_subdirectory(src/ui)
add_subdirectory(src/core)
add_subdirectory(src/app)
