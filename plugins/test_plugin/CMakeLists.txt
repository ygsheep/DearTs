cmake_minimum_required(VERSION 3.20)

# 测试插件项目
project(test_plugin VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 输出目录
set(PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/plugins")

# 创建 DLL
add_library(${PROJECT_NAME} SHARED
    test_plugin_export.cpp
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}  # 为了找到 core/plugin/plugin.h
)

# 链接库（如果需要）
# target_link_libraries(${PROJECT_NAME} PRIVATE dearts_core)

# Windows 特定设置
if(WIN32)
    # 导出所有符号
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        DEARTS_PLUGIN_API_EXPORTS
    )
endif()

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
)

# Windows DLL 输出为 .dll
# Linux 共享库输出为 .so
# macOS 共享库输出为 .dylib

message(STATUS "Test plugin configured:")
message(STATUS "  Output directory: ${PLUGIN_OUTPUT_DIR}")
message(STATUS "  Library name: ${PROJECT_NAME}")
