cmake_minimum_required(VERSION 3.20)
project(BuddyMemory LANGUAGES CXX)

# ---- C++ standard ----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Options ----
option(ENABLE_ASAN "Enable AddressSanitizer" ON)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# ---- Library: allocators ----
add_library(allocators
    src/buddy.cpp
    src/freelist.cpp
    src/superblock.cpp
)

target_include_directories(allocators
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

# Warnings (feel free to add -Wpedantic later)
target_compile_options(allocators PRIVATE
    -Wall -Wextra
)

# ---- Executable: demo ----
add_executable(demo
    examples/main.cpp
)

target_link_libraries(demo PRIVATE allocators)

# ---- Sanitizers (macOS clang) ----
if (ENABLE_ASAN)
    target_compile_options(allocators PRIVATE -fsanitize=address)
    target_link_options(allocators PRIVATE -fsanitize=address)
    target_compile_options(demo PRIVATE -fsanitize=address)
    target_link_options(demo PRIVATE -fsanitize=address)
endif()

if (ENABLE_UBSAN)
    target_compile_options(allocators PRIVATE -fsanitize=undefined)
    target_link_options(allocators PRIVATE -fsanitize=undefined)
    target_compile_options(demo PRIVATE -fsanitize=undefined)
    target_link_options(demo PRIVATE -fsanitize=undefined)
endif()
