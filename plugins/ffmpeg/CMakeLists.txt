# FFmpeg Plugin CMakeLists.txt

cmake_minimum_required(VERSION 3.20)

# 设置插件源文件
set(FFMPEG_PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ffmpeg_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ffmpeg_view.cpp
)

# 设置插件头文件
set(FFMPEG_PLUGIN_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ffmpeg_plugin.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ffmpeg_view.hpp
)

# 创建插件静态库
add_library(ffmpeg_plugin STATIC ${FFMPEG_PLUGIN_SOURCES})

# 设置包含目录
target_include_directories(ffmpeg_plugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/third_party/SDL/include
    ${CMAKE_SOURCE_DIR}/third_party/imgui
    ${CMAKE_SOURCE_DIR}/plugins/toast_notification/include
)

# 链接 FFmpeg（如果可用）
if(TARGET ffmpeg_interface)
    target_link_libraries(ffmpeg_plugin PUBLIC ffmpeg_interface)
    message(STATUS "FFmpeg Plugin: FFmpeg support enabled")
else()
    message(WARNING "FFmpeg Plugin: FFmpeg not available, plugin will have limited functionality")
endif()

# 链接必要的库
target_link_libraries(ffmpeg_plugin PRIVATE
    imgui_full
    liblogger
)

# Windows 特定配置
if(WIN32)
    target_compile_definitions(ffmpeg_plugin PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# 编译选项
if(MSVC)
    target_compile_options(ffmpeg_plugin PRIVATE
        /W4
        /wd4996  # 禁用 "sprintf": 函数不安全
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ffmpeg_plugin PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# 设置输出目录
set_target_properties(ffmpeg_plugin PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# 安装规则（可选）
install(TARGETS ffmpeg_plugin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

message(STATUS "FFmpeg Plugin: Configured successfully")
