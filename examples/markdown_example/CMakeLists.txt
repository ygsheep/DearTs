cmake_minimum_required(VERSION 3.20)

# Markdown 渲染示例可执行文件
add_executable(markdown_example
    main.cpp
)

# 包含目录
target_include_directories(markdown_example PRIVATE
    ${DEARTS_ROOT}
    ${DEARTS_ROOT}/core
    ${DEARTS_ROOT}/core/event
    ${DEARTS_ROOT}/core/content
    ${DEARTS_ROOT}/core/ui
    ${DEARTS_ROOT}/core/plugin
    ${DEARTS_ROOT}/core/config
    ${DEARTS_ROOT}/core/tasks
    ${DEARTS_ROOT}/core/app
    ${DEARTS_ROOT}/lib/liblogger
    ${DEARTS_ROOT}/third_party/SDL/include
    ${DEARTS_ROOT}/third_party/imgui
    ${DEARTS_ROOT}/third_party/imgui/backends
    ${DEARTS_ROOT}/third_party/imgui_markdown
)

# 源文件（最小依赖）
target_sources(markdown_example PRIVATE
    ${DEARTS_ROOT}/core/content/settings.cpp
    ${DEARTS_ROOT}/core/content/commands.cpp
    ${DEARTS_ROOT}/core/content/tools.cpp
    ${DEARTS_ROOT}/core/content/callbacks.cpp
    ${DEARTS_ROOT}/core/content/project_manager.cpp
    ${DEARTS_ROOT}/core/config/config_manager.cpp
    ${DEARTS_ROOT}/core/plugin/plugin.cpp
    ${DEARTS_ROOT}/core/plugin/plugin_loader.cpp
    # Dependency management system (required by plugin.cpp)
    ${DEARTS_ROOT}/core/plugin/version.cpp
    ${DEARTS_ROOT}/core/plugin/version_range.cpp
    ${DEARTS_ROOT}/core/plugin/plugin_dependency.cpp
    ${DEARTS_ROOT}/core/plugin/dependency_resolver.cpp
    ${DEARTS_ROOT}/core/ui/imgui_layer.cpp
    ${DEARTS_ROOT}/core/ui/view.cpp
    ${DEARTS_ROOT}/core/ui/view_manager.cpp
    ${DEARTS_ROOT}/core/ui/layout_manager.cpp
    ${DEARTS_ROOT}/core/ui/title_bar.cpp
    ${DEARTS_ROOT}/core/ui/imgui_extensions.cpp
    ${DEARTS_ROOT}/core/ui/theme_manager.cpp
    ${DEARTS_ROOT}/core/ui/shortcut_manager.cpp
    ${DEARTS_ROOT}/core/ui/task_widget.cpp
    ${DEARTS_ROOT}/core/ui/icon_font.cpp
    ${DEARTS_ROOT}/core/tasks/task_manager.cpp
    ${DEARTS_ROOT}/core/app/application.cpp
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
    ${DEARTS_ROOT}/third_party/imgui/imconfig.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui_user_config.h
    COPYONLY
)

# ================ 复制资源文件 ================

# 定义资源目录
set(RESOURCE_SOURCE_DIR ${DEARTS_ROOT}/resources)
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
