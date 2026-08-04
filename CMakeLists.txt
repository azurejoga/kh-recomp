cmake_minimum_required(VERSION 3.20)
project(KHRecomp C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)

# Path to the gbarecomp platform core.
set(GBARECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/gbarecomp" CACHE PATH "Path to gbarecomp platform core")

if(NOT EXISTS "${GBARECOMP_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "GBARECOMP_ROOT does not point to a gbarecomp checkout: ${GBARECOMP_ROOT}")
endif()

# Pull in the platform core targets. This makes gbarecomp_runtime,
# gbarecomp_debug, gbarecomp_gba, gbarecomp_armv4t available.
add_subdirectory("${GBARECOMP_ROOT}" gbarecomp_build EXCLUDE_FROM_ALL)

# Generated recompiled cart code (from gba_recompile).
file(GLOB KH_GENERATED_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/*.cpp")

add_executable(KHRecomp
    src/main.cpp
    ${KH_GENERATED_SOURCES}
)

target_include_directories(KHRecomp PRIVATE
    "${GBARECOMP_ROOT}/src/armv4t"
    "${GBARECOMP_ROOT}/src/gba"
    "${GBARECOMP_ROOT}/src/runtime"
    "${GBARECOMP_ROOT}/src/debug"
    src
    generated
)

target_link_libraries(KHRecomp PRIVATE
    # --start-group/--end-group lets ld re-iterate the gbarecomp static
    # libs to resolve their mutual references.
    "-Wl,--start-group"
    gbarecomp_runtime
    gbarecomp_debug
    gbarecomp_gba
    gbarecomp_armv4t
    "-Wl,--end-group"
)

target_compile_definitions(KHRecomp PRIVATE
    GBARECOMP_DEFAULT_DEBUG_PORT=19843
    GBARECOMP_WINDOW_TITLE="KHRecomp"
    GBARECOMP_DEFAULT_GAME_CONFIG="game.toml"
)

if(MSVC)
    target_link_options(KHRecomp PRIVATE /STACK:16777216)
elseif(MINGW OR WIN32)
    target_link_options(KHRecomp PRIVATE -Wl,--stack,16777216)
endif()

# prism.dll is an import-lib runtime dependency (linked via prism/lib/libprism.a):
# the launcher window + screen-reader bridge need it sitting next to the exe.
# Copy it automatically so a fresh build "just runs".
set(PRISM_BIN "${CMAKE_CURRENT_SOURCE_DIR}/prism/bin")
if(WIN32 AND EXISTS "${PRISM_BIN}/prism.dll")
    add_custom_command(TARGET KHRecomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PRISM_BIN}/prism.dll" "$<TARGET_FILE_DIR:KHRecomp>/prism.dll"
        COMMENT "Copying prism.dll next to KHRecomp.exe (runtime import-lib dep)")
endif()

# Playable package: the exe reads game.toml + save from its own directory.
# game.build.toml is self-contained (paths relative to the exe dir); the ROM
# (.gba) and BIOS (.bin) are NOT versioned (copyright) — the user drops them
# next to the exe with the expected names.
set(GAME_BUILD_TOML "${CMAKE_CURRENT_SOURCE_DIR}/game.build.toml")
if(WIN32 AND EXISTS "${GAME_BUILD_TOML}")
    add_custom_command(TARGET KHRecomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GAME_BUILD_TOML}" "$<TARGET_FILE_DIR:KHRecomp>/game.toml"
        COMMENT "Copying self-contained game.toml next to KHRecomp.exe")
endif()
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/roms/khcom_eur.sav")
    add_custom_command(TARGET KHRecomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/roms/khcom_eur.sav"
            "$<TARGET_FILE_DIR:KHRecomp>/khcom_eur.sav"
        COMMENT "Copying khcom_eur.sav next to KHRecomp.exe")
endif()
