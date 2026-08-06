# Navigation Plugin CMakeLists.txt

cmake_minimum_required(VERSION 3.20)

# 设置源文件
set(NAVIGATION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/navigation_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/sidebar_view.cpp
)

# 设置头文件目录
set(NAVIGATION_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 创建导航插件静态库
add_library(navigation_plugin STATIC ${NAVIGATION_SOURCES})

target_include_directories(navigation_plugin
    PUBLIC
        ${NAVIGATION_INCLUDE_DIRS}
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

# 链接必要的库
target_link_libraries(navigation_plugin
    PUBLIC
        imgui
        SDL3-static
        deartsdl_logger
)

# 设置 C++20 标准
target_compile_features(navigation_plugin PUBLIC cxx_std_20)

message(STATUS "Navigation plugin configured")

if(MSVC)
    target_compile_options(navigation_plugin PRIVATE /utf-8)
endif()
