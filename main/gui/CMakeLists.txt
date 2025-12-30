# DearTs 应用程序 CMakeLists.txt
# 基于 DearTs 框架的 GUI 应用

cmake_minimum_required(VERSION 3.20)
project(DearTs)
# DearTs 可执行文件
add_executable(${PROJECT_NAME}
    include/dearts_application.hpp
    source/main.cpp
    source/dearts_application.cpp
    # 插件源文件
    ${CMAKE_SOURCE_DIR}/plugins/builtin/include/builtin_plugin.hpp
    ${CMAKE_SOURCE_DIR}/plugins/builtin/include/views/data_inspector_view.hpp
    ${CMAKE_SOURCE_DIR}/plugins/builtin/source/builtin_plugin.cpp
    # Settings 插件
    ${CMAKE_SOURCE_DIR}/plugins/settings/include/settings_plugin.hpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/include/views/settings_view.hpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/include/widgets/theme_settings_widget.hpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/settings_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/settings_view.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/toast_settings_widget.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/toast_settings_widget.hpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/widgets/theme_settings_widget.cpp
    # LoggerViewer 插件
    ${CMAKE_SOURCE_DIR}/plugins/logger_viewer/include/logger_viewer_plugin.hpp
    ${CMAKE_SOURCE_DIR}/plugins/logger_viewer/include/views/logger_viewer_view.hpp
    ${CMAKE_SOURCE_DIR}/plugins/logger_viewer/source/logger_viewer_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/logger_viewer/source/logger_viewer_view.cpp
    # Navigation 插件
    ${CMAKE_SOURCE_DIR}/plugins/navigation/include/navigation_plugin.hpp
    ${CMAKE_SOURCE_DIR}/plugins/navigation/include/views/sidebar_view.hpp
    ${CMAKE_SOURCE_DIR}/plugins/navigation/source/navigation_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/navigation/source/sidebar_view.cpp
    # Toast Notification 插件
    ${CMAKE_SOURCE_DIR}/plugins/toast_notification/include/toast.hpp
    ${CMAKE_SOURCE_DIR}/plugins/toast_notification/include/toast_manager.hpp
    ${CMAKE_SOURCE_DIR}/plugins/toast_notification/include/toast_view.hpp
    ${CMAKE_SOURCE_DIR}/plugins/toast_notification/include/toast_plugin.hpp
    ${CMAKE_SOURCE_DIR}/plugins/toast_notification/source/toast_manager.cpp
    ${CMAKE_SOURCE_DIR}/plugins/toast_notification/source/toast_plugin.cpp
    # Command Palette 插件
    ${CMAKE_SOURCE_DIR}/plugins/command_palette/include/command_palette_plugin.hpp
    ${CMAKE_SOURCE_DIR}/plugins/command_palette/include/command_palette_view.hpp
    ${CMAKE_SOURCE_DIR}/plugins/command_palette/source/command_palette_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/command_palette/source/command_palette_view.cpp
    # Windows 资源文件（仅 Windows）
    ${CMAKE_SOURCE_DIR}/resources/dearts.rc
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/core/event
    ${CMAKE_SOURCE_DIR}/core/content
    ${CMAKE_SOURCE_DIR}/core/ui
    ${CMAKE_SOURCE_DIR}/core/plugin
    ${CMAKE_SOURCE_DIR}/core/config
    ${CMAKE_SOURCE_DIR}/core/tasks
    ${CMAKE_SOURCE_DIR}/core/utils
    ${CMAKE_SOURCE_DIR}/core/app
    ${CMAKE_SOURCE_DIR}/lib/liblogger
    ${CMAKE_SOURCE_DIR}/plugins/builtin/include
    ${CMAKE_SOURCE_DIR}/plugins/settings/include
    ${CMAKE_SOURCE_DIR}/plugins/logger_viewer/include
    ${CMAKE_SOURCE_DIR}/plugins/navigation/include
    ${CMAKE_SOURCE_DIR}/plugins/toast_notification/include
    ${CMAKE_SOURCE_DIR}/plugins/command_palette/include
    ${CMAKE_SOURCE_DIR}/third_party/SDL/include
    ${CMAKE_SOURCE_DIR}/third_party/freetype/include
    ${CMAKE_SOURCE_DIR}/third_party/imgui
    ${CMAKE_SOURCE_DIR}/third_party/imgui/backends
    ${CMAKE_SOURCE_DIR}/third_party/imgui/misc/freetype
    ${CMAKE_SOURCE_DIR}/third_party/implot
)

# 源文件（新架构实现）
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/core/content/settings.cpp
    ${CMAKE_SOURCE_DIR}/core/content/commands.cpp
    ${CMAKE_SOURCE_DIR}/core/content/tools.cpp
    ${CMAKE_SOURCE_DIR}/core/content/callbacks.cpp
    ${CMAKE_SOURCE_DIR}/core/content/project_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/config/config_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/plugin.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/imgui_layer.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/view.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/view_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/layout_manager.cpp
    # Command Palette 现在是插件，移到了 plugins/command_palette/
    # ${CMAKE_SOURCE_DIR}/core/ui/command_palette.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/title_bar.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/imgui_extensions.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/theme_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/shortcut_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/task_widget.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/icon_font.cpp
    ${CMAKE_SOURCE_DIR}/core/tasks/task_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/app/application.cpp
    ${CMAKE_SOURCE_DIR}/core/utils/file_dialog.cpp
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
        imgui
        imgui_sdl3
        implot
        SDL3-static
        deartsdl_logger
        freetype
        lunasvg::lunasvg
)

# Windows 特定设置
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
    )
endif()

# 定义资源目录
set(RESOURCE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
set(RESOURCE_DEST_DIR ${CMAKE_BINARY_DIR}/bin/resources)

# 复制所有资源文件到输出目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # 创建 resources 目录
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}
    # 复制字体文件
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${RESOURCE_SOURCE_DIR}/fonts
        ${RESOURCE_DEST_DIR}/fonts
    # 复制图标文件
    COMMAND ${CMAKE_COMMAND} -E copy
        ${RESOURCE_SOURCE_DIR}/icon.ico
        ${CMAKE_BINARY_DIR}/bin/icon.ico
    COMMENT "Copying resources to output directory"
)

# 显示资源目录信息
message(STATUS "Resource directories:")
message(STATUS "  Source: ${RESOURCE_SOURCE_DIR}")
message(STATUS "  Destination: ${RESOURCE_DEST_DIR}")

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 编译器特定设置
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${PROJECT_NAME} PRIVATE _DEBUG)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    endif()
endif()

message(STATUS "DearTs Application Configuration:")
message(STATUS "  Target: ${PROJECT_NAME}")
message(STATUS "  Framework: DearTs Core")
message(STATUS "  ImGui: Integrated")
message(STATUS "  Theme System: Enabled")
message(STATUS "  Shortcut System: Enabled")
message(STATUS "  Command Palette: Enabled")
message(STATUS "  Custom Title Bar: Enabled")
message(STATUS "  Docking: Enabled")
message(STATUS "  Config Management: Enabled")
