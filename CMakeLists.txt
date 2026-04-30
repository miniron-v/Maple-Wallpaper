cmake_minimum_required(VERSION 3.21)
project(MapleWallpaper VERSION 0.1.0 LANGUAGES CXX)

# ── C++ standard ────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Static CRT (/MT, /MTd) ─────────────────────────────────────
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ── Dependencies (vcpkg) ───────────────────────────────────────
find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# ── core_lib (STATIC) ──────────────────────────────────────────
add_library(core_lib STATIC
    src/core/core.cpp
    src/render/render.cpp
    src/character/character.cpp
    src/api/api.cpp
    src/cache/cache.cpp
    src/platform/platform.cpp
    src/platform/tray.cpp
    src/platform/wallpaper_host.cpp
    src/platform/session_monitor.cpp
)

target_include_directories(core_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(core_lib PUBLIC
    raylib
    glfw
    cpr::cpr
    nlohmann_json::nlohmann_json
    opengl32
    gdi32
    winmm
    user32
    shell32
    wtsapi32
)

# ── MapleWallpaper.exe ──────────────────────────────────────────
add_executable(MapleWallpaper
    src/wallpaper/main.cpp
)

target_link_libraries(MapleWallpaper PRIVATE core_lib)

set_target_properties(MapleWallpaper PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# ── MapleWallpaperConfig.exe ────────────────────────────────────
add_executable(MapleWallpaperConfig
    src/config_app/main.cpp
)

target_link_libraries(MapleWallpaperConfig PRIVATE core_lib)

# ── CPack (ZIP) ─────────────────────────────────────────────────
install(TARGETS MapleWallpaper MapleWallpaperConfig
    RUNTIME DESTINATION bin
)

install(DIRECTORY assets/ DESTINATION assets)

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "MapleWallpaper")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-win64")
include(CPack)
