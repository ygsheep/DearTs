# DearTs Core Library CMake Configuration
# 
# 核心库构建配置 - 包含事件系统、窗口管理器、应用程序管理器等核心组件
# 提供项目的基础架构和核心功能支持
# 
# @author DearTs Team
# @version 1.0.0
# @date 2025

cmake_minimum_required(VERSION 3.20)

# 核心库项目信息
project(DearTsCore
    VERSION 1.0.0
    DESCRIPTION "DearTs Core Library - Event System, Window Manager, Application Manager"
    LANGUAGES CXX
)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译器特定设置
if(MSVC)
    add_compile_options(/W4 /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 调试模式设置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEARTS_DEBUG)
endif()

# 核心库源文件
set(CORE_SOURCES
    # 核心系统
    core.cpp
    
    # 事件系统
    events/event_system.cpp
    
    # 应用程序管理
    app/application_manager.cpp
    
    # 窗口管理
    window/window_manager.cpp
    window/window_base.cpp
    window/main_window.cpp
    window/layouts/layout_base.cpp
    window/layouts/title_bar_layout.cpp
    window/layouts/layout_manager.cpp
    window/layouts/sidebar_layout.cpp
    window/layouts/pomodoro_layout.cpp
    
    # 渲染系统
    render/renderer.cpp
    render/renderer_adapter.cpp
    
    # 输入系统
    input/input_manager.cpp
    
    # 资源管理
    resource/resource_manager.cpp
    resource/font_resource.cpp
    
    # 音频系统
    audio/audio_manager.cpp
    
    # 工具类
    utils/config_manager.cpp
    utils/file_utils.cpp
    utils/string_utils.cpp
    utils/profiler.cpp
)

# 核心库头文件
set(CORE_HEADERS
    # 核心系统
    core.h
    
    # 事件系统
    events/event_system.h
    
    # 应用程序管理
    app/application_manager.h
    
    # 窗口管理
    window/window_manager.h
    window/window_base.h
    window/main_window.h
    window/layouts/layout_base.h
    window/layouts/title_bar_layout.h
    window/layouts/layout_manager.h
    window/layouts/sidebar_layout.h
    window/layouts/pomodoro_layout.h
    
    # 渲染系统
    render/renderer.h
    
    # 输入系统
    input/input_manager.h
    
    # 资源管理
    resource/resource_manager.h
    resource/font_resource.h
    
    # 音频系统
    audio/audio_manager.h
    
    # 设计模式
    patterns/singleton.h
    
    # 工具类
    # utils/logger.h  # Removed logger header
    utils/config_manager.h
    utils/file_utils.h
    utils/string_utils.h
    utils/profiler.h
)

# 创建核心库
add_library(DearTsCore STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# 设置库别名
add_library(DearTs::Core ALIAS DearTsCore)

# 设置目标属性
set_target_properties(DearTsCore PROPERTIES
    OUTPUT_NAME "dearts_core"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# 包含目录
target_include_directories(DearTsCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/events>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/patterns>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/source
)

# 查找依赖库
find_package(PkgConfig QUIET)

# SDL2依赖
if(WIN32)
    set(SDL2_DIR "${CMAKE_SOURCE_DIR}/lib/third_party/SDL2")
    set(SDL2_TTF_DIR "${CMAKE_SOURCE_DIR}/lib/third_party/SDL2_ttf")
    set(SDL2_IMAGE_DIR "${CMAKE_SOURCE_DIR}/lib/third_party/SDL2_image")
    set(SDL2_MIXER_DIR "${CMAKE_SOURCE_DIR}/lib/third_party/SDL2_mixer")
    
    find_library(SDL2_LIBRARY
        NAMES SDL2
        PATHS ${SDL2_DIR}/lib/x64
        NO_DEFAULT_PATH
    )
    
    find_library(SDL2_TTF_LIBRARY
        NAMES SDL2_ttf
        PATHS ${SDL2_TTF_DIR}/lib/x64
        NO_DEFAULT_PATH
    )
    
    find_library(SDL2_IMAGE_LIBRARY
        NAMES SDL2_image
        PATHS ${SDL2_IMAGE_DIR}/lib/x64
        NO_DEFAULT_PATH
    )
    
    find_library(SDL2_MIXER_LIBRARY
        NAMES SDL2_mixer
        PATHS ${SDL2_MIXER_DIR}/lib/x64
        NO_DEFAULT_PATH
    )
    
    target_include_directories(DearTsCore PRIVATE
        ${SDL2_DIR}/include
        ${SDL2_TTF_DIR}/include
        ${SDL2_IMAGE_DIR}/include
        ${SDL2_MIXER_DIR}/include
    )
else()
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
    
    target_include_directories(DearTsCore PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
    )
endif()

# ImGui依赖
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")
target_include_directories(DearTsCore PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# 链接库
target_link_libraries(DearTsCore
    PUBLIC
        # 标准库
        $<$<PLATFORM_ID:Windows>:ws2_32>
        $<$<PLATFORM_ID:Windows>:winmm>
        $<$<PLATFORM_ID:Windows>:version>
        $<$<PLATFORM_ID:Windows>:imm32>
        $<$<PLATFORM_ID:Windows>:setupapi>
        
        # 跨平台库
        $<$<NOT:$<PLATFORM_ID:Windows>>:pthread>
        $<$<NOT:$<PLATFORM_ID:Windows>>:dl>
        
    PRIVATE
        # SDL2库
        $<$<PLATFORM_ID:Windows>:${SDL2_LIBRARY}>
        $<$<PLATFORM_ID:Windows>:${SDL2_TTF_LIBRARY}>
        $<$<PLATFORM_ID:Windows>:${SDL2_IMAGE_LIBRARY}>
        $<$<PLATFORM_ID:Windows>:${SDL2_MIXER_LIBRARY}>
        $<$<NOT:$<PLATFORM_ID:Windows>>:${SDL2_LIBRARIES}>
        $<$<NOT:$<PLATFORM_ID:Windows>>:${SDL2_TTF_LIBRARIES}>
)

# 编译定义
target_compile_definitions(DearTsCore
    PUBLIC
        DEARTS_CORE_EXPORTS
        $<$<CONFIG:Debug>:DEARTS_DEBUG>
        $<$<CONFIG:Release>:DEARTS_RELEASE>
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
    PRIVATE
        DEARTS_CORE_BUILDING
)

# 编译选项
if(MSVC)
    target_compile_options(DearTsCore PRIVATE
        /W4
        /utf-8
        /permissive-
        /Zc:__cplusplus
        $<$<CONFIG:Debug>:/Od /Zi>
        $<$<CONFIG:Release>:/O2 /DNDEBUG>
    )
else()
    target_compile_options(DearTsCore PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

# 预编译头文件
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(DearTsCore PRIVATE
        <memory>
        <string>
        <vector>
        <map>
        <unordered_map>
        <functional>
        <algorithm>
        <iostream>
        <fstream>
        <sstream>
        <thread>
        <mutex>
        <atomic>
        <chrono>
        <filesystem>
    )
endif()

# 安装配置
install(TARGETS DearTsCore
    EXPORT DearTsCoreTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# 安装头文件
install(DIRECTORY events/ DESTINATION include/dearts/events
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(DIRECTORY app/ DESTINATION include/dearts/app
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(DIRECTORY window/ DESTINATION include/dearts/window
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(DIRECTORY render/ DESTINATION include/dearts/render
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(DIRECTORY input/ DESTINATION include/dearts/input
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(DIRECTORY resource/ DESTINATION include/dearts/resource
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(DIRECTORY audio/ DESTINATION include/dearts/audio
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(DIRECTORY utils/ DESTINATION include/dearts/utils
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# 导出目标
install(EXPORT DearTsCoreTargets
    FILE DearTsCoreTargets.cmake
    NAMESPACE DearTs::
    DESTINATION lib/cmake/DearTsCore
)

# 生成配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/DearTsCoreConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/DearTsCoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DearTsCoreConfig.cmake"
    INSTALL_DESTINATION lib/cmake/DearTsCore
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/DearTsCoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/DearTsCoreConfigVersion.cmake"
    DESTINATION lib/cmake/DearTsCore
)

# 测试配置
if(DEARTS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例配置
if(DEARTS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 文档配置
if(DEARTS_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(core_docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# 打包配置
set(CPACK_PACKAGE_NAME "DearTsCore")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DearTs Core Library")
set(CPACK_PACKAGE_VENDOR "DearTs Team")
set(CPACK_PACKAGE_CONTACT "dearts@example.com")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
endif()

include(CPack)

# 输出构建信息
message(STATUS "DearTs Core Library Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Tests: ${DEARTS_BUILD_TESTS}")
message(STATUS "  Examples: ${DEARTS_BUILD_EXAMPLES}")
message(STATUS "  Documentation: ${DEARTS_BUILD_DOCS}")