# Test Dependency Plugin CMake Configuration

cmake_minimum_required(VERSION 3.20)

# 测试依赖插件项目
project(test_dependency_plugin VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 输出目录
set(PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/plugins")

# 创建 DLL
add_library(${PROJECT_NAME} SHARED
    test_dependency_plugin_export.cpp
    # ContentRegistry 源文件
    ${CMAKE_SOURCE_DIR}/core/content/commands.cpp
    # 依赖管理源文件
    ${CMAKE_SOURCE_DIR}/core/plugin/version.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/version_range.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/plugin_dependency.cpp
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}  # 为了找到 core/plugin/plugin.h
    ${CMAKE_SOURCE_DIR}/lib/liblogger  # 为了找到 liblogger/logger.h
)

# 链接库（如果需要）
target_link_libraries(${PROJECT_NAME} PRIVATE deartsdl_logger)

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

# 自动复制到 bin/plugins/ 目录（用于自动发现）
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/plugins/"
        COMMENT "Copying ${PROJECT_NAME}.dll to bin/$<CONFIG>/plugins/"
    )
elseif(APPLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/plugins/lib${PROJECT_NAME}.dylib"
        COMMENT "Copying lib${PROJECT_NAME}.dylib to bin/$<CONFIG>/plugins/"
    )
else()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CMAKE_BINARY_DIR}/bin/plugins/"
        COMMENT "Copying lib${PROJECT_NAME}.so to bin/plugins/"
    )
endif()

message(STATUS "Test Dependency Plugin configured:")
message(STATUS "  Output directory: ${PLUGIN_OUTPUT_DIR}")
message(STATUS "  Library name: ${PROJECT_NAME}")
