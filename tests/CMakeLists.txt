# DearTs Framework Testing Configuration
# This file sets up the testing infrastructure using Google Test and ImGui Test Engine

cmake_minimum_required(VERSION 3.20)

# == Options ==
option(BUILD_UNIT_TESTS "Build unit tests" ON)
option(BUILD_UI_TESTS "Build UI automation tests" ON)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

# == Google Test Integration ==
if(BUILD_UNIT_TESTS)
    message(STATUS "Building unit tests with Google Test")

    # Add Google Test subdirectory
    if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/googletest/CMakeLists.txt)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "Disable installation of Google Test" FORCE)
        # Specify binary directory since source is outside tests/
        add_subdirectory(
            ${CMAKE_SOURCE_DIR}/third_party/googletest
            ${CMAKE_BINARY_DIR}/third_party/googletest
            EXCLUDE_FROM_ALL
        )
        message(STATUS "Google Test found and configured")
    else()
        message(FATAL_ERROR "Google Test not found in third_party/googletest!")
    endif()

    # == Collect test sources ==
    file(GLOB_RECURSE UNIT_TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp"
    )

    # Filter out main files if any (we use gtest_main)
    list(FILTER UNIT_TEST_SOURCES EXCLUDE REGEX ".*_main\\.cpp$")

    # == Unit Tests Executable ==
    add_executable(dearts_unit_tests
        ${UNIT_TEST_SOURCES}
    )

    target_include_directories(dearts_unit_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/lib
    )

    target_link_libraries(dearts_unit_tests
        PRIVATE
        gtest
        gtest_main
        # Add DearTs core libraries here
        # e.g., DearTsCore
    )

    # Enable coverage if requested
    if(ENABLE_COVERAGE)
        target_compile_options(dearts_unit_tests PRIVATE
            $<$<CXX_COMPILER_ID:GNU>:--coverage>
            $<$<CXX_COMPILER_ID:Clang>:--coverage>
        )
        target_link_options(dearts_unit_tests PRIVATE
            $<$<CXX_COMPILER_ID:GNU>:--coverage>
            $<$<CXX_COMPILER_ID:Clang>:--coverage>
        )
        message(STATUS "Code coverage enabled for unit tests")
    endif()

    # == CTest Integration ==
    include(GoogleTest)
    gtest_discover_tests(dearts_unit_tests)

    message(STATUS "Unit tests configured: dearts_unit_tests")
endif()

# == ImGui Test Engine Integration ==
if(BUILD_UI_TESTS)
    message(STATUS "Building UI automation tests with ImGui Test Engine")

    # Add ImGui Test Engine
    if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine/imgui_test_engine)
        # Assuming ImGui Test Engine is already set up as a library
        # You may need to add_subdirectory or link against it

        # Collect UI test sources
        file(GLOB_RECURSE UI_TEST_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.cpp"
        )

        # UI Tests Executable
        add_executable(dearts_ui_tests
            ${UI_TEST_SOURCES}
        )

        target_include_directories(dearts_ui_tests
            PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/core
            ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine
            ${CMAKE_SOURCE_DIR}/third_party/imgui
        )

        target_link_libraries(dearts_ui_tests
            PRIVATE
            imgui_test_engine
            imgui
            # Link against DearTs core and SDL
        )

        message(STATUS "UI tests configured: dearts_ui_tests")
    else()
        message(WARNING "ImGui Test Engine not found, UI tests disabled")
    endif()
endif()

# == Testing Summary ==
message(STATUS "")
message(STATUS "=== DearTs Testing Configuration ===")
message(STATUS "  Build Unit Tests:    ${BUILD_UNIT_TESTS}")
message(STATUS "  Build UI Tests:      ${BUILD_UI_TESTS}")
message(STATUS "  Enable Coverage:     ${ENABLE_COVERAGE}")
message(STATUS "=====================================")
message(STATUS "")
