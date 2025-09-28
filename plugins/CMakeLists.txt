# DearTs Plugin System CMake Configuration
# 
# 插件系统构建配置 - 支持内置插件和外部插件的编译管理
# 提供插件发现、编译、链接和安装的完整构建流程
# 
# @author DearTs Team
# @version 1.0.0
# @date 2025

cmake_minimum_required(VERSION 3.20)

# 插件系统配置
set(DEARTS_PLUGIN_VERSION_MAJOR 1)
set(DEARTS_PLUGIN_VERSION_MINOR 0)
set(DEARTS_PLUGIN_VERSION_PATCH 0)
set(DEARTS_PLUGIN_VERSION "${DEARTS_PLUGIN_VERSION_MAJOR}.${DEARTS_PLUGIN_VERSION_MINOR}.${DEARTS_PLUGIN_VERSION_PATCH}")

# 插件目录配置
set(DEARTS_BUILTIN_PLUGINS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/builtin")
set(DEARTS_EXTERNAL_PLUGINS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(DEARTS_PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/plugins")
set(DEARTS_PLUGIN_CONFIG_DIR "${CMAKE_BINARY_DIR}/plugins/config")

# 创建插件输出目录
file(MAKE_DIRECTORY ${DEARTS_PLUGIN_OUTPUT_DIR})
file(MAKE_DIRECTORY ${DEARTS_PLUGIN_CONFIG_DIR})

# 插件编译选项
option(DEARTS_BUILD_BUILTIN_PLUGINS "Build builtin plugins" ON)
option(DEARTS_BUILD_EXTERNAL_PLUGINS "Build external plugins" ON)
option(DEARTS_PLUGIN_SHARED_LIBS "Build plugins as shared libraries" ON)
option(DEARTS_PLUGIN_STATIC_RUNTIME "Link plugins with static runtime" OFF)

# 插件编译标志
set(DEARTS_PLUGIN_COMPILE_FLAGS)
set(DEARTS_PLUGIN_LINK_FLAGS)

if(WIN32)
    list(APPEND DEARTS_PLUGIN_COMPILE_FLAGS "/utf-8")
    if(DEARTS_PLUGIN_STATIC_RUNTIME)
        list(APPEND DEARTS_PLUGIN_COMPILE_FLAGS "/MT$<$<CONFIG:Debug>:d>")
    else()
        list(APPEND DEARTS_PLUGIN_COMPILE_FLAGS "/MD$<$<CONFIG:Debug>:d>")
    endif()
else()
    list(APPEND DEARTS_PLUGIN_COMPILE_FLAGS "-fPIC")
    list(APPEND DEARTS_PLUGIN_COMPILE_FLAGS "-Wall" "-Wextra")
endif()

# 插件宏定义
set(DEARTS_PLUGIN_DEFINITIONS
    DEARTS_PLUGIN_VERSION_MAJOR=${DEARTS_PLUGIN_VERSION_MAJOR}
    DEARTS_PLUGIN_VERSION_MINOR=${DEARTS_PLUGIN_VERSION_MINOR}
    DEARTS_PLUGIN_VERSION_PATCH=${DEARTS_PLUGIN_VERSION_PATCH}
    DEARTS_PLUGIN_VERSION="${DEARTS_PLUGIN_VERSION}"
)

if(DEARTS_PLUGIN_SHARED_LIBS)
    list(APPEND DEARTS_PLUGIN_DEFINITIONS DEARTS_PLUGIN_SHARED)
else()
    list(APPEND DEARTS_PLUGIN_DEFINITIONS DEARTS_PLUGIN_STATIC)
endif()

# 插件依赖库
set(DEARTS_PLUGIN_DEPENDENCIES
    libdearts
    imgui
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

if(WIN32)
    list(APPEND DEARTS_PLUGIN_DEPENDENCIES
        user32
        gdi32
        shell32
        ole32
        oleaut32
        imm32
        winmm
        version
    )
else()
    list(APPEND DEARTS_PLUGIN_DEPENDENCIES
        ${CMAKE_DL_LIBS}
        pthread
    )
endif()

# 插件包含目录
set(DEARTS_PLUGIN_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/lib/libdearts/include
    ${CMAKE_SOURCE_DIR}/main/gui/include
    ${CMAKE_SOURCE_DIR}/plugins/builtin
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
)

# 插件创建函数
function(dearts_add_plugin PLUGIN_NAME)
    set(options BUILTIN EXTERNAL SHARED STATIC)
    set(oneValueArgs VERSION DESCRIPTION OUTPUT_NAME)
    set(multiValueArgs SOURCES HEADERS DEPENDENCIES INCLUDE_DIRS COMPILE_DEFINITIONS)
    cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # 设置默认值
    if(NOT PLUGIN_VERSION)
        set(PLUGIN_VERSION "1.0.0")
    endif()
    
    if(NOT PLUGIN_DESCRIPTION)
        set(PLUGIN_DESCRIPTION "DearTs Plugin: ${PLUGIN_NAME}")
    endif()
    
    if(NOT PLUGIN_OUTPUT_NAME)
        set(PLUGIN_OUTPUT_NAME ${PLUGIN_NAME})
    endif()

    # 确定库类型
    set(PLUGIN_LIBRARY_TYPE SHARED)
    if(PLUGIN_STATIC OR NOT DEARTS_PLUGIN_SHARED_LIBS)
        set(PLUGIN_LIBRARY_TYPE STATIC)
    endif()

    # 创建插件目标
    if(PLUGIN_LIBRARY_TYPE STREQUAL "SHARED")
        add_library(${PLUGIN_NAME} SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})
    else()
        add_library(${PLUGIN_NAME} STATIC ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})
    endif()

    # 设置目标属性
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        OUTPUT_NAME ${PLUGIN_OUTPUT_NAME}
        VERSION ${PLUGIN_VERSION}
        SOVERSION ${DEARTS_PLUGIN_VERSION_MAJOR}
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
    )

    # 设置输出目录
    if(PLUGIN_BUILTIN)
        set_target_properties(${PLUGIN_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${DEARTS_PLUGIN_OUTPUT_DIR}/builtin
            LIBRARY_OUTPUT_DIRECTORY ${DEARTS_PLUGIN_OUTPUT_DIR}/builtin
            ARCHIVE_OUTPUT_DIRECTORY ${DEARTS_PLUGIN_OUTPUT_DIR}/builtin
        )
    else()
        set_target_properties(${PLUGIN_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${DEARTS_PLUGIN_OUTPUT_DIR}/external
            LIBRARY_OUTPUT_DIRECTORY ${DEARTS_PLUGIN_OUTPUT_DIR}/external
            ARCHIVE_OUTPUT_DIRECTORY ${DEARTS_PLUGIN_OUTPUT_DIR}/external
        )
    endif()

    # 包含目录
    target_include_directories(${PLUGIN_NAME} PRIVATE
        ${DEARTS_PLUGIN_INCLUDE_DIRS}
        ${PLUGIN_INCLUDE_DIRS}
    )

    # 编译定义
    target_compile_definitions(${PLUGIN_NAME} PRIVATE
        ${DEARTS_PLUGIN_DEFINITIONS}
        DEARTS_PLUGIN_NAME="${PLUGIN_NAME}"
        DEARTS_PLUGIN_VERSION="${PLUGIN_VERSION}"
        DEARTS_PLUGIN_DESCRIPTION="${PLUGIN_DESCRIPTION}"
        ${PLUGIN_COMPILE_DEFINITIONS}
    )

    # 编译选项
    target_compile_options(${PLUGIN_NAME} PRIVATE ${DEARTS_PLUGIN_COMPILE_FLAGS})

    # 链接库
    target_link_libraries(${PLUGIN_NAME} PRIVATE
        ${DEARTS_PLUGIN_DEPENDENCIES}
        ${PLUGIN_DEPENDENCIES}
    )

    # 链接选项
    if(DEARTS_PLUGIN_LINK_FLAGS)
        target_link_options(${PLUGIN_NAME} PRIVATE ${DEARTS_PLUGIN_LINK_FLAGS})
    endif()

    # Windows特定设置
    if(WIN32 AND PLUGIN_LIBRARY_TYPE STREQUAL "SHARED")
        target_compile_definitions(${PLUGIN_NAME} PRIVATE DEARTS_PLUGIN_EXPORTS)
    endif()

    # 调试信息
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${PLUGIN_NAME} PRIVATE DEARTS_DEBUG)
    endif()

    # 安装规则
    if(PLUGIN_BUILTIN)
        install(TARGETS ${PLUGIN_NAME}
            RUNTIME DESTINATION plugins/builtin
            LIBRARY DESTINATION plugins/builtin
            ARCHIVE DESTINATION plugins/builtin
        )
    else()
        install(TARGETS ${PLUGIN_NAME}
            RUNTIME DESTINATION plugins/external
            LIBRARY DESTINATION plugins/external
            ARCHIVE DESTINATION plugins/external
        )
    endif()

    # 插件信息输出
    message(STATUS "Plugin configured: ${PLUGIN_NAME}")
    message(STATUS "  - Type: ${PLUGIN_LIBRARY_TYPE}")
    message(STATUS "  - Version: ${PLUGIN_VERSION}")
    message(STATUS "  - Description: ${PLUGIN_DESCRIPTION}")
    message(STATUS "  - Category: ${PLUGIN_BUILTIN}${PLUGIN_EXTERNAL}")
endfunction()

# 内置插件构建
if(DEARTS_BUILD_BUILTIN_PLUGINS)
    message(STATUS "Building builtin plugins...")
    
    # 演示插件
    if(EXISTS "${DEARTS_BUILTIN_PLUGINS_DIR}/demo_plugin.hpp")
        dearts_add_plugin(demo_plugin BUILTIN
            VERSION "1.0.0"
            DESCRIPTION "DearTs Framework Demo Plugin"
            SOURCES
                "${DEARTS_BUILTIN_PLUGINS_DIR}/demo_plugin.cpp"
                "${DEARTS_BUILTIN_PLUGINS_DIR}/builtin_plugin.cpp"
            HEADERS
                "${DEARTS_BUILTIN_PLUGINS_DIR}/demo_plugin.hpp"
                "${DEARTS_BUILTIN_PLUGINS_DIR}/builtin_plugin.hpp"
        )
    endif()
    
    # 自动发现其他内置插件
    file(GLOB BUILTIN_PLUGIN_DIRS "${DEARTS_BUILTIN_PLUGINS_DIR}/*/")
    foreach(PLUGIN_DIR ${BUILTIN_PLUGIN_DIRS})
        get_filename_component(PLUGIN_NAME ${PLUGIN_DIR} NAME)
        
        # 查找插件配置文件
        if(EXISTS "${PLUGIN_DIR}/CMakeLists.txt")
            message(STATUS "Found builtin plugin: ${PLUGIN_NAME}")
            add_subdirectory(${PLUGIN_DIR})
        endif()
    endforeach()
endif()

# 外部插件构建
if(DEARTS_BUILD_EXTERNAL_PLUGINS AND EXISTS ${DEARTS_EXTERNAL_PLUGINS_DIR})
    message(STATUS "Building external plugins...")
    
    # 自动发现外部插件
    file(GLOB EXTERNAL_PLUGIN_DIRS "${DEARTS_EXTERNAL_PLUGINS_DIR}/*/")
    foreach(PLUGIN_DIR ${EXTERNAL_PLUGIN_DIRS})
        get_filename_component(PLUGIN_NAME ${PLUGIN_DIR} NAME)
        
        # 查找插件配置文件
        if(EXISTS "${PLUGIN_DIR}/CMakeLists.txt")
            message(STATUS "Found external plugin: ${PLUGIN_NAME}")
            add_subdirectory(${PLUGIN_DIR})
        endif()
    endforeach()
endif()

# 插件配置文件生成
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/plugin_config.h.in"
    "${CMAKE_BINARY_DIR}/include/dearts/plugins/plugin_config.h"
    @ONLY
)

# 插件清单生成
set(PLUGIN_MANIFEST_FILE "${DEARTS_PLUGIN_OUTPUT_DIR}/plugin_manifest.json")
file(WRITE ${PLUGIN_MANIFEST_FILE} "{\n")
file(APPEND ${PLUGIN_MANIFEST_FILE} "  \"version\": \"${DEARTS_PLUGIN_VERSION}\",\n")
file(APPEND ${PLUGIN_MANIFEST_FILE} "  \"build_time\": \"${CMAKE_TIMESTAMP}\",\n")
file(APPEND ${PLUGIN_MANIFEST_FILE} "  \"builtin_plugins\": [],\n")
file(APPEND ${PLUGIN_MANIFEST_FILE} "  \"external_plugins\": []\n")
file(APPEND ${PLUGIN_MANIFEST_FILE} "}\n")

# 插件系统信息输出
message(STATUS "")
message(STATUS "=== DearTs Plugin System Configuration ===")
message(STATUS "Plugin System Version: ${DEARTS_PLUGIN_VERSION}")
message(STATUS "Builtin Plugins: ${DEARTS_BUILD_BUILTIN_PLUGINS}")
message(STATUS "External Plugins: ${DEARTS_BUILD_EXTERNAL_PLUGINS}")
message(STATUS "Shared Libraries: ${DEARTS_PLUGIN_SHARED_LIBS}")
message(STATUS "Static Runtime: ${DEARTS_PLUGIN_STATIC_RUNTIME}")
message(STATUS "Output Directory: ${DEARTS_PLUGIN_OUTPUT_DIR}")
message(STATUS "Config Directory: ${DEARTS_PLUGIN_CONFIG_DIR}")
message(STATUS "============================================")
message(STATUS "")

# 插件测试（可选）
if(DEARTS_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif()

# 插件文档生成（可选）
if(DEARTS_BUILD_DOCS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs")
    add_subdirectory(docs)
endif()