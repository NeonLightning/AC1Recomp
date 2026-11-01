cmake_minimum_required(VERSION 3.20)
set(PSXRECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/psxrecomp" CACHE PATH
    "Path to psxrecomp")
if(NOT EXISTS "${PSXRECOMP_ROOT}/runtime/runtime.cmake")
    message(FATAL_ERROR
        "PSXRecomp runtime is missing: ${PSXRECOMP_ROOT}/runtime/runtime.cmake\n"
        "This project expects the psxrecomp framework checkout at ${PSXRECOMP_ROOT}.\n"
        "Run: git submodule update --init --recursive\n"
        "If psxrecomp/ is not a submodule, set -DPSXRECOMP_ROOT=<path-to-psxrecomp>.")
endif()

project(AC1 LANGUAGES C CXX)

# Keep fetched dependency sources and their CMake build trees outside the
# disposable per-build directory. FETCHCONTENT_BASE_DIR can still be overridden.
if(NOT DEFINED FETCHCONTENT_BASE_DIR OR FETCHCONTENT_BASE_DIR STREQUAL "" OR
    FETCHCONTENT_BASE_DIR STREQUAL "${CMAKE_BINARY_DIR}/_deps")
    set(_ac1_fetchcontent_key
        "${CMAKE_SYSTEM_NAME}-${CMAKE_C_COMPILER_ID}-${CMAKE_C_COMPILER_VERSION}-${CMAKE_GENERATOR}")
    string(MD5 _ac1_build_tree_id "${CMAKE_BINARY_DIR}")
    string(APPEND _ac1_fetchcontent_key "-${_ac1_build_tree_id}")
    if(CMAKE_BUILD_TYPE)
        string(APPEND _ac1_fetchcontent_key "-${CMAKE_BUILD_TYPE}")
    endif()
    if(DEFINED ENV{XDG_CACHE_HOME} AND NOT "$ENV{XDG_CACHE_HOME}" STREQUAL "")
        set(_ac1_fetchcontent_root "$ENV{XDG_CACHE_HOME}")
    elseif(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
        set(_ac1_fetchcontent_root "$ENV{HOME}/.cache")
    elseif(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
        set(_ac1_fetchcontent_root "$ENV{USERPROFILE}/AppData/Local")
    else()
        set(_ac1_fetchcontent_root "${CMAKE_BINARY_DIR}")
    endif()
    set(FETCHCONTENT_BASE_DIR
        "${_ac1_fetchcontent_root}/ac1recomp/fetchcontent/${_ac1_fetchcontent_key}"
        CACHE PATH "FetchContent cache for AC1 dependencies" FORCE)
endif()

# --- Opt-in product surfaces (OFF by default in the framework) -------------
# Scaffold prompts / flags fill these. Without recomp-ui, PSX_RECOMP_UI is OFF.
# The wizard (WIZARD_CMAKE_BLOCK / ENABLE_SETUP_WIZARD) is the first-run disc
# picker; releases are compiled from the committed generated/ C, so it never
# needs to generate or rebuild anything on a player's machine.
# recomp-ui submodule present (PSX_RECOMP_UI defaults ON).
# Full netplay UI — enable after adding recomp-ui + testing:
# if(EXISTS "${PSXRECOMP_ROOT}/lib/recomp-net/CMakeLists.txt")
#     set(PSX_NETPLAY ON CACHE BOOL "…" FORCE)
# endif()
set(PSX_SETUP_WIZARD ON CACHE BOOL
    "Advertise first-run setup wizard + Generate & rebuild in recomp-ui" FORCE)
include("${PSXRECOMP_ROOT}/runtime/runtime.cmake")

# This project's own mod catalog. The FRAMEWORK stages it -- together with its
# builtin psx.* packages -- into <exe-dir>/mods/bundled, and refuses to
# configure if a catalog exists here but is not declared. Never copy it by
# hand: five titles did, and framework 4cc04be3's rename of the staged
# directory broke all five silently (bead beads-eio.3.101).
set(PSX_PRELOADED_MODS "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded")
if(NOT IS_DIRECTORY "${PSX_PRELOADED_MODS}")
    set(PSX_PRELOADED_MODS "NONE")
endif()

psxrecomp_add_game_runtime(psx-armored-core
    PRELOADED_MODS_DIR "${PSX_PRELOADED_MODS}"
    # ENABLE_NETPLAY_IF_PRESENT
    # NETPLAY_LOBBY_URL "ws://netplay.retcomm.net:8765"
    ENABLE_SETUP_WIZARD
    WINDOW_TITLE "AC1"
    DEFAULT_GAME_CONFIG_PATH "games/armored-core/game.toml"
    MAX_PLAYERS 1
    # LAUNCHER_BOXART "${CMAKE_CURRENT_SOURCE_DIR}/launcher_assets/img/boxart.tga"
    APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/psxrecomp.ico"
    # Committed game C (psxrecomp_cli.py generate; git add generated). Release
    # CI configures with -DPSXRECOMP_REQUIRE_GAME_C=ON so a checkout without
    # it fails instead of building a setup host.
    GEN_MARKER "games/armored-core/generated/SCUS_941.82_dispatch.c"
    GEN_FULL_GLOB "games/armored-core/generated/SCUS_941.82_full_*.c"
    CODEGEN_SETUP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/games/armored-core/codegen_setup.c"
    CODEGEN_SETUP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/games/armored-core"
)

psxrecomp_add_game_runtime(psx-project-phantasma
    PRELOADED_MODS_DIR "${PSX_PRELOADED_MODS}"
    ENABLE_SETUP_WIZARD
    WINDOW_TITLE "Armored Core: Project Phantasma"
    EXE_NAME "ArmoredCoreProjectPhantasma"
    DEFAULT_GAME_CONFIG_PATH "games/project-phantasma/game.toml"
    MAX_PLAYERS 2
    APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/psxrecomp.ico"
    GEN_MARKER "games/project-phantasma/generated/SLUS_006.70_dispatch.c"
    GEN_FULL_GLOB "games/project-phantasma/generated/SLUS_006.70_full_*.c"
    CODEGEN_SETUP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/games/project-phantasma/codegen_setup.c"
    CODEGEN_SETUP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/games/project-phantasma"
)

psxrecomp_add_game_runtime(psx-master-of-arena
    PRELOADED_MODS_DIR "${PSX_PRELOADED_MODS}"
    ENABLE_SETUP_WIZARD
    WINDOW_TITLE "Armored Core: Master of Arena"
    EXE_NAME "ArmoredCoreMasterOfArena"
    DEFAULT_GAME_CONFIG_PATH "games/master-of-arena/game.toml"
    MAX_PLAYERS 1
    APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/psxrecomp.ico"
    GEN_MARKER "games/master-of-arena/generated/SLUS_010.30_dispatch.c"
    GEN_FULL_GLOB "games/master-of-arena/generated/SLUS_010.30_full_*.c"
    CODEGEN_SETUP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/games/master-of-arena/codegen_setup.c"
    CODEGEN_SETUP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/games/master-of-arena"
)

# Hub launcher: game picker, per-game disc and shared settings.
set(AC_IMGUI "${CMAKE_CURRENT_SOURCE_DIR}/recomp-ui/src/third_party/imgui")
add_executable(psx-launcher
    launcher/main.cpp
    recomp-ui/src/third_party/tinyfiledialogs.c
    ${AC_IMGUI}/imgui.cpp
    ${AC_IMGUI}/imgui_draw.cpp
    ${AC_IMGUI}/imgui_tables.cpp
    ${AC_IMGUI}/imgui_widgets.cpp
    ${AC_IMGUI}/backends/imgui_impl_sdl3.cpp
    ${AC_IMGUI}/backends/imgui_impl_opengl3.cpp)
set_target_properties(psx-launcher PROPERTIES
    OUTPUT_NAME ArmoredCoreLauncher CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(psx-launcher PRIVATE ${AC_IMGUI} ${AC_IMGUI}/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/recomp-ui/src/third_party)
target_link_libraries(psx-launcher PRIVATE ${PSX_SDL_LIBRARIES} ${CMAKE_DL_LIBS})

# Parallel target builds each stage into these; create them before any runs.
foreach(_PSX_STAGE_DIR bios assets mods)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${_PSX_STAGE_DIR}")
endforeach()

# Stage each game's config next to its settings in <build>/games/<id>/.
foreach(_PSX_AC_CONFIG game.toml game_options.toml)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/games/armored-core")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/games/armored-core/${_PSX_AC_CONFIG}"
        "${CMAKE_CURRENT_BINARY_DIR}/games/armored-core/${_PSX_AC_CONFIG}"
        COPYONLY)
endforeach()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/games/project-phantasma")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/games/project-phantasma/game.toml"
    "${CMAKE_CURRENT_BINARY_DIR}/games/project-phantasma/game.toml"
    COPYONLY)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/games/master-of-arena")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/games/master-of-arena/game.toml"
    "${CMAKE_CURRENT_BINARY_DIR}/games/master-of-arena/game.toml"
    COPYONLY)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui")
    add_custom_command(TARGET psx-armored-core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui"
            "$<TARGET_FILE_DIR:psx-armored-core>/assets/ui"
        COMMENT "Staging custom UI assets"
        VERBATIM)
endif()
