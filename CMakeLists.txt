cmake_minimum_required(VERSION 3.20)
project(wombocombo-gameserver VERSION 0.2.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Options ──────────────────────────────────────────
option(ENABLE_ASAN  "Enable AddressSanitizer"  OFF)
option(ENABLE_TSAN  "Enable ThreadSanitizer"   OFF)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
    message(STATUS "AddressSanitizer ENABLED")
endif()

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
    message(STATUS "ThreadSanitizer ENABLED")
endif()

# ── Dependencies ─────────────────────────────────────

# nlohmann/json
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(json)
endif()

# OpenSSL (for JWT HMAC-SHA256 verification)
find_package(OpenSSL REQUIRED)

# hiredis (Redis client)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(HIREDIS hiredis)
endif()
if(NOT HIREDIS_FOUND)
    find_library(HIREDIS_LIBRARIES hiredis)
    find_path(HIREDIS_INCLUDE_DIRS hiredis/hiredis.h)
endif()
if(NOT HIREDIS_LIBRARIES)
    message(FATAL_ERROR "hiredis not found. Install with: apt install libhiredis-dev")
endif()

# zlib for uWebSockets
find_package(ZLIB REQUIRED)

# uSockets — built as a static lib
set(USOCKETS_DIR ${CMAKE_SOURCE_DIR}/third_party/uWebSockets/uSockets)
file(GLOB USOCKETS_SRC ${USOCKETS_DIR}/src/*.c ${USOCKETS_DIR}/src/eventing/*.c ${USOCKETS_DIR}/src/crypto/*.c)

add_library(uSockets STATIC ${USOCKETS_SRC})
target_include_directories(uSockets PUBLIC ${USOCKETS_DIR}/src)
target_compile_definitions(uSockets PUBLIC LIBUS_NO_SSL)

# uWebSockets include path
set(UWS_INCLUDE ${CMAKE_SOURCE_DIR}/third_party/uWebSockets/src)

# ── Main executable ──────────────────────────────────
file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(gameserver ${SOURCES})

target_include_directories(gameserver PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${UWS_INCLUDE}
    ${HIREDIS_INCLUDE_DIRS}
)

target_link_libraries(gameserver PRIVATE
    uSockets
    nlohmann_json::nlohmann_json
    OpenSSL::Crypto
    ${HIREDIS_LIBRARIES}
    ZLIB::ZLIB
    pthread
)

# Warnings
target_compile_options(gameserver PRIVATE -Wall -Wextra -Wpedantic)

# ── Install ──────────────────────────────────────────
install(TARGETS gameserver DESTINATION bin)
