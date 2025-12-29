# 日志查看器插件

cmake_minimum_required(VERSION 3.20)

# 插件源文件
set(LOGGER_VIEWER_SOURCES
    include/logger_viewer_plugin.hpp
    include/views/logger_viewer_view.hpp
    source/logger_viewer_plugin.cpp
    source/logger_viewer_view.cpp
)

# 如果作为独立项目编译
if(CMAKE_PROJECT_NAME STREQUAL "logger_viewer")
    # 添加到主应用程序
    message(STATUS "Configuring LoggerViewer Plugin as standalone")
endif()

message(STATUS "LoggerViewer Plugin configured")
