# WutheringWaves Plugin CMakeLists.txt (静态编译版本)

cmake_minimum_required(VERSION 3.20)

# 设置源文件
set(WUTHERING_WAVES_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/wuthering_waves_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/gacha_record_view.cpp
)

# 设置头文件目录
set(WUTHERING_WAVES_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 创建鸣潮抽卡记录插件静态库
add_library(wuthering_waves_plugin STATIC ${WUTHERING_WAVES_SOURCES})

target_include_directories(wuthering_waves_plugin
    PUBLIC
        ${WUTHERING_WAVES_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/core/app
        ${CMAKE_SOURCE_DIR}/core/ui
        ${CMAKE_SOURCE_DIR}/core/content
        ${CMAKE_SOURCE_DIR}/core/event
        ${CMAKE_SOURCE_DIR}/core/plugin
        ${CMAKE_SOURCE_DIR}/core/config
        ${CMAKE_SOURCE_DIR}/core/tasks
        ${CMAKE_SOURCE_DIR}/lib/liblogger/include
        ${CMAKE_SOURCE_DIR}/third_party/imgui
        ${SDL3_INCLUDE_DIRS}
)

# 链接必要的库（注意：不要链接 SDL3，由主程序统一管理）
target_link_libraries(wuthering_waves_plugin
    PUBLIC
        imgui
        deartsdl_logger
)

# 设置 C++20 标准
target_compile_features(wuthering_waves_plugin PUBLIC cxx_std_20)

# MSVC: 源文件为 UTF-8（无 BOM），必须显式指定 /utf-8，否则中文按 ANSI 代码页误读
if(MSVC)
    target_compile_options(wuthering_waves_plugin PRIVATE /utf-8)
endif()

message(STATUS "WutheringWaves plugin configured (static)")
