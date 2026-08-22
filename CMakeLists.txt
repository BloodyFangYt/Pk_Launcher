cmake_minimum_required(VERSION 3.16)
project(PkLauncher VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql Concurrent)

# Project directories
set(PKLAUNCHER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PKLAUNCHER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PKLAUNCHER_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")

include_directories(${PKLAUNCHER_INCLUDE_DIR})

# Source files (excluding main.cpp — used for both the app and test library)
file(GLOB_RECURSE PKLAUNCHER_LIB_SOURCES
    "${PKLAUNCHER_SRC_DIR}/core/*.cpp"
    "${PKLAUNCHER_SRC_DIR}/launcher/*.cpp"
    "${PKLAUNCHER_SRC_DIR}/network/*.cpp"
    "${PKLAUNCHER_SRC_DIR}/ui/*.cpp"
    "${PKLAUNCHER_SRC_DIR}/utils/*.cpp"
)

file(GLOB_RECURSE PKLAUNCHER_HEADERS
    "${PKLAUNCHER_INCLUDE_DIR}/*.h"
    "${PKLAUNCHER_INCLUDE_DIR}/**/*.h"
)

# Resources
file(GLOB PKLAUNCHER_RESOURCES
    "${PKLAUNCHER_RESOURCES_DIR}/*.qrc"
)

# Static library for shared sources (used by main app + tests)
add_library(PkLauncherLib STATIC
    ${PKLAUNCHER_LIB_SOURCES}
    ${PKLAUNCHER_HEADERS}
)

target_link_libraries(PkLauncherLib PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
)

# Main executable
add_executable(PkLauncher
    src/main.cpp
    ${PKLAUNCHER_RESOURCES}
)

target_link_libraries(PkLauncher PRIVATE PkLauncherLib)

# Qt6::WebEngineWidgets if needed
# target_link_libraries(PkLauncher PRIVATE Qt6::WebEngineWidgets)

# Installation
install(TARGETS PkLauncher
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

# ── Package configuration (CPack) ──────────────────────────────────────────────
set(CPACK_PACKAGE_NAME "PkLauncher")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Minecraft Launcher - Cross-platform desktop app")
set(CPACK_PACKAGE_VENDOR "PK Launcher Team")
set(CPACK_PACKAGE_CONTACT "support@pklauncher.dev")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://pklauncher.dev")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# Default generators per platform
if(WIN32)
    set(CPACK_GENERATOR "NSIS;TGZ")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
else()
    set(CPACK_GENERATOR "TGZ;ZIP")
endif()

# ── NSIS (Windows) ─────────────────────────────────────────────────────────
set(CPACK_NSIS_DISPLAY_NAME "PK Launcher")
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\PkLauncher.exe")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$DESKTOP\\PK Launcher.lnk' '$INSTDIR\\bin\\PkLauncher.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$DESKTOP\\PK Launcher.lnk'")
set(CPACK_NSIS_URL_INFO_ABOUT "https://pklauncher.dev")

# ── DragNDrop / DMG (macOS) ───────────────────────────────────────────────
set(CPACK_DMG_VOLUME_NAME "PK Launcher")
set(CPACK_DMG_FORMAT "ULZO")

# ── Linux packaging helpers ────────────────────────────────────────────────
# To build AppImage: cmake --build . --target appimage
# Requires linuxdeployqt or appimagetool in PATH
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)

include(CPack)

# ── Tests ──────────────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build unit and integration tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()