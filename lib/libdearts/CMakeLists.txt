cmake_minimum_required(VERSION 3.16)

# 设置项目名称和版本
project(libdearts VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 查找依赖包
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)

# 收集源文件
file(GLOB_RECURSE LIBDEARTS_SOURCES
    "source/*.cpp"
    "source/*.c"
)

# 收集头文件
file(GLOB_RECURSE LIBDEARTS_HEADERS
    "include/*.hpp"
    "include/*.h"
)

# 创建静态库
add_library(libdearts STATIC
    ${LIBDEARTS_SOURCES}
    ${LIBDEARTS_HEADERS}
)

# 设置库的别名
add_library(dearts::libdearts ALIAS libdearts)

# 设置包含目录
target_include_directories(libdearts
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

# 链接依赖库
target_link_libraries(libdearts
    PUBLIC
        nlohmann_json::nlohmann_json
        fmt::fmt
)

# 设置编译定义
target_compile_definitions(libdearts
    PUBLIC
        DEARTS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        DEARTS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        DEARTS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        DEARTS_VERSION="${PROJECT_VERSION}"
    PRIVATE
        DEARTS_BUILDING_LIBRARY
)

# 平台特定设置
if(WIN32)
    target_compile_definitions(libdearts PUBLIC DEARTS_PLATFORM_WINDOWS)
    target_link_libraries(libdearts PRIVATE kernel32 user32 shell32)
elseif(APPLE)
    target_compile_definitions(libdearts PUBLIC DEARTS_PLATFORM_MACOS)
elseif(UNIX)
    target_compile_definitions(libdearts PUBLIC DEARTS_PLATFORM_LINUX)
    target_link_libraries(libdearts PRIVATE dl pthread)
endif()

# 调试/发布配置
target_compile_definitions(libdearts
    PUBLIC
        $<$<CONFIG:Debug>:DEARTS_DEBUG>
        $<$<CONFIG:Release>:DEARTS_RELEASE>
        $<$<CONFIG:RelWithDebInfo>:DEARTS_RELEASE>
        $<$<CONFIG:MinSizeRel>:DEARTS_RELEASE>
)

# 设置输出目录
set_target_properties(libdearts PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 设置调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(libdearts PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/Zi>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-g>
    )
endif()

# 启用位置无关代码（用于共享库）
set_target_properties(libdearts PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# 安装配置
include(GNUInstallDirs)

# 安装库文件
install(TARGETS libdearts
    EXPORT deartsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装头文件
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# 创建并安装配置文件
include(CMakePackageConfigHelpers)

# 生成版本文件
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/deartsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 生成配置文件
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deartsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/deartsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dearts
)

# 安装配置文件
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/deartsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/deartsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dearts
)

# 安装导出文件
install(EXPORT deartsTargets
    FILE deartsTargets.cmake
    NAMESPACE dearts::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dearts
)

# 创建pkg-config文件
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/libdearts.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libdearts.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libdearts.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# 添加测试（如果启用）
option(DEARTS_BUILD_TESTS "Build tests" OFF)
if(DEARTS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 添加示例（如果启用）
option(DEARTS_BUILD_EXAMPLES "Build examples" OFF)
if(DEARTS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 生成编译数据库（用于IDE支持）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 打印配置信息
message(STATUS "DearTs libdearts configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Build tests: ${DEARTS_BUILD_TESTS}")
message(STATUS "  Build examples: ${DEARTS_BUILD_EXAMPLES}")