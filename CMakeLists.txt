cmake_minimum_required(VERSION 3.28)

project(hindsight
    VERSION 0.0.1
    DESCRIPTION "Zero-config analyzer for Node.js projects"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# hindsight uses C++23 library features that libstdc++ only ships from GCC 14 on:
# std::print, std::generator, std::ranges::to, and deducing this. GCC 13 rejects all four.
# See docs/DESIGN-DECISIONS.md, ADR-5.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14)
    message(FATAL_ERROR
        "hindsight requires GCC 14 or newer, found GCC ${CMAKE_CXX_COMPILER_VERSION}.\n"
        "Reason: the C++23 library features this project uses (std::print, std::generator, "
        "std::ranges::to) and deducing this are not available in GCC 13.\n"
        "Fix on Ubuntu 24.04: sudo apt install g++-14\n"
        "Then configure with: cmake --preset debug   (the presets select g++-14), "
        "or pass -DCMAKE_CXX_COMPILER=g++-14 explicitly.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(hindsight_warnings INTERFACE)
target_compile_options(hindsight_warnings INTERFACE -Wall -Wextra -Wpedantic)

# Headers live next to their module sources under src/, included as <module/header.hpp>.
add_library(hindsight_core INTERFACE)
target_include_directories(hindsight_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(hindsight_core INTERFACE hindsight_warnings)

add_executable(hindsight src/main.cpp)
target_link_libraries(hindsight PRIVATE hindsight_core)

enable_testing()
add_subdirectory(tests)
