cmake_minimum_required(VERSION 3.20)

# Markdown 渲染示例可执行文件
add_executable(markdown_example
    main.cpp
)

# 包含目录
target_include_directories(markdown_example PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/core/event
    ${CMAKE_SOURCE_DIR}/core/content
    ${CMAKE_SOURCE_DIR}/core/ui
    ${CMAKE_SOURCE_DIR}/core/plugin
    ${CMAKE_SOURCE_DIR}/core/config
    ${CMAKE_SOURCE_DIR}/core/tasks
    ${CMAKE_SOURCE_DIR}/core/app
    ${CMAKE_SOURCE_DIR}/lib/liblogger
    ${CMAKE_SOURCE_DIR}/third_party/SDL/include
    ${CMAKE_SOURCE_DIR}/third_party/imgui
    ${CMAKE_SOURCE_DIR}/third_party/imgui/backends
    ${CMAKE_SOURCE_DIR}/third_party/imgui_markdown
)

# 源文件（最小依赖）
target_sources(markdown_example PRIVATE
    ${CMAKE_SOURCE_DIR}/core/content/settings.cpp
    ${CMAKE_SOURCE_DIR}/core/content/commands.cpp
    ${CMAKE_SOURCE_DIR}/core/content/tools.cpp
    ${CMAKE_SOURCE_DIR}/core/content/callbacks.cpp
    ${CMAKE_SOURCE_DIR}/core/content/project_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/config/config_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/plugin.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/plugin_loader.cpp
    # Dependency management system (required by plugin.cpp)
    ${CMAKE_SOURCE_DIR}/core/plugin/version.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/version_range.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/plugin_dependency.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/dependency_resolver.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/imgui_layer.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/view.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/view_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/layout_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/title_bar.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/imgui_extensions.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/theme_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/shortcut_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/task_widget.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/icon_font.cpp
    ${CMAKE_SOURCE_DIR}/core/tasks/task_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/app/application.cpp
)

# 链接库
target_link_libraries(markdown_example PRIVATE
    imgui
    imgui_sdl3
    SDL3-static
    deartsdl_logger
)

# Windows特定设置
if(WIN32)
    target_compile_definitions(markdown_example PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
    )
    # 链接 shell32 用于 ShellExecuteA
    target_link_libraries(markdown_example PRIVATE shell32)
endif()

# 拷贝ImGui配置文件
configure_file(
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imconfig.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui_user_config.h
    COPYONLY
)

# ================ 复制资源文件 ================

# 定义资源目录
set(RESOURCE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
set(RESOURCE_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/resources)

# 复制字体文件
add_custom_command(TARGET markdown_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${RESOURCE_SOURCE_DIR}/fonts
        ${RESOURCE_DEST_DIR}/fonts
    COMMENT "Copying font files to output directory"
)

message(STATUS "Markdown example configured: markdown_example")
message(STATUS "Resource files will be copied to: ${RESOURCE_DEST_DIR}")
