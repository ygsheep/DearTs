# DearTs Framework Testing Configuration
# This file sets up the testing infrastructure using Google Test and ImGui Test Engine

cmake_minimum_required(VERSION 3.20)

include_directories(mocks/)

# == Options ==
option(BUILD_UNIT_TESTS "Build unit tests" ON)
option(BUILD_INTEGRATION_TESTS "Build integration tests" ON)
option(BUILD_UI_TESTS "Build UI automation tests" ON)  # DISABLED: ImGui Test Engine incompatible with current ImGui version
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
        ${CMAKE_SOURCE_DIR}/lib/liblogger
    )

    # 源文件（新架构实现）
    target_sources(dearts_unit_tests PRIVATE
            ${CMAKE_SOURCE_DIR}/core/config/config_manager.cpp
            ${CMAKE_SOURCE_DIR}/core/tasks/task_manager.cpp
            ${CMAKE_SOURCE_DIR}/core/plugin/plugin.cpp
    )

    target_link_libraries(dearts_unit_tests
        PRIVATE
        gtest
        gtest_main
        deartsdl_logger
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

# == Integration Tests ==
if(BUILD_INTEGRATION_TESTS)
    message(STATUS "Building integration tests with Google Test")

    # == Collect integration test sources ==
    file(GLOB_RECURSE INTEGRATION_TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cpp"
    )

    # Filter out main files if any
    list(FILTER INTEGRATION_TEST_SOURCES EXCLUDE REGEX ".*_main\\.cpp$")

    # == Integration Tests Executable ==
    add_executable(dearts_integration_tests
        ${INTEGRATION_TEST_SOURCES}
    )

    target_include_directories(dearts_integration_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/lib/liblogger
        ${CMAKE_SOURCE_DIR}/third_party/json/include
    )

    # 源文件（集成测试需要的核心实现）
    target_sources(dearts_integration_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/core/config/config_manager.cpp
        ${CMAKE_SOURCE_DIR}/core/tasks/task_manager.cpp
        ${CMAKE_SOURCE_DIR}/core/plugin/plugin.cpp
        ${CMAKE_SOURCE_DIR}/core/content/commands.cpp
        ${CMAKE_SOURCE_DIR}/core/content/settings.cpp
    )

    target_link_libraries(dearts_integration_tests
        PRIVATE
        gtest
        gtest_main
        deartsdl_logger
    )

    # == CTest Integration ==
    include(GoogleTest)
    gtest_discover_tests(dearts_integration_tests)

    message(STATUS "Integration tests configured: dearts_integration_tests")
endif()

# == ImGui Test Engine Integration ==
if(BUILD_UI_TESTS)
    message(STATUS "Building UI automation tests with ImGui Test Engine")

    # Add ImGui Test Engine
    if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine/imgui_test_engine)
        # Collect UI test sources
        file(GLOB_RECURSE UI_TEST_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.cpp"
        )

        # Filter out main files if any
        list(FILTER UI_TEST_SOURCES EXCLUDE REGEX ".*_main\\.cpp$")
        list(FILTER UI_TEST_SOURCES EXCLUDE REGEX ".*sdl3_diagnostic.cpp$")
        list(FILTER UI_TEST_SOURCES EXCLUDE REGEX ".*test_runner_simple.cpp$")

        # UI Tests Executable
        add_executable(dearts_ui_tests
            ${UI_TEST_SOURCES}
        )

        target_include_directories(dearts_ui_tests
            PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/core
            ${CMAKE_SOURCE_DIR}/lib
            ${CMAKE_SOURCE_DIR}/main/gui/include
            ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine
            ${CMAKE_SOURCE_DIR}/third_party/imgui
            ${CMAKE_SOURCE_DIR}/third_party/imgui/backends
            ${CMAKE_SOURCE_DIR}/third_party/SDL/include
        )

        target_link_libraries(dearts_ui_tests
            PRIVATE
            imgui_test_engine
            imgui
            imgui_sdl3
            SDL3-static
            deartsdl_logger
            freetype
            lunasvg
        )

        # Define ImGui Test Engine enable flag
        target_compile_definitions(dearts_ui_tests
            PRIVATE
            IMGUI_TEST_ENGINE_ENABLE
        )

        message(STATUS "UI tests configured: dearts_ui_tests")
    else()
        message(WARNING "ImGui Test Engine not found, UI tests disabled")
    endif()
endif()
# SDL3 诊断工具（用于调试 SDL3 初始化问题）
if(BUILD_UI_TESTS AND EXISTS ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine/imgui_test_engine)
    add_executable(sdl3_diagnostic ${CMAKE_CURRENT_SOURCE_DIR}/ui/sdl3_diagnostic.cpp)
    target_link_libraries(sdl3_diagnostic PRIVATE SDL3-static)
    if(WIN32)
        target_link_libraries(sdl3_diagnostic PRIVATE 
            kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 
            version uuid advapi32 setupapi shell32 dinput8)
    endif()
    message(STATUS "SDL3 diagnostic tool configured")
endif()


# == Testing Summary ==
message(STATUS "")
message(STATUS "=== DearTs Testing Configuration ===")
message(STATUS "  Build Unit Tests:       ${BUILD_UNIT_TESTS}")
message(STATUS "  Build Integration Tests: ${BUILD_INTEGRATION_TESTS}")
message(STATUS "  Build UI Tests:         ${BUILD_UI_TESTS}")
message(STATUS "  Enable Coverage:        ${ENABLE_COVERAGE}")
message(STATUS "==========================================")
message(STATUS "")
