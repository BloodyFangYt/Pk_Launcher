# tests/CMakeLists.txt — Qt Test targets for PkLauncher
# This file is included from the root CMakeLists.txt when BUILD_TESTS=ON.

find_package(Qt6 REQUIRED COMPONENTS Test)

# Helper macro to add a Qt Test target
macro(pklauncher_add_test TEST_NAME TEST_SOURCES)
    add_executable(${TEST_NAME} ${TEST_SOURCES})
    target_link_libraries(${TEST_NAME} PRIVATE
        Qt6::Test
        PkLauncherLib
    )
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

# ── Unit tests ────────────────────────────────────────────────────────────────
pklauncher_add_test(
    test_settings
    test_settings.cpp
)

pklauncher_add_test(
    test_instance_manager
    test_instance_manager.cpp
)

pklauncher_add_test(
    test_launcher_core
    test_launcher_core.cpp
)

pklauncher_add_test(
    test_download_pipeline
    test_download_pipeline.cpp
)

pklauncher_add_test(
    test_instance_lifecycle
    test_instance_lifecycle.cpp
)

pklauncher_add_test(
    test_auth_manager
    test_auth_manager.cpp
)

pklauncher_add_test(
    test_e2e_real
    test_e2e_real.cpp
)

pklauncher_add_test(
    test_all_versions
    test_all_versions.cpp
)
