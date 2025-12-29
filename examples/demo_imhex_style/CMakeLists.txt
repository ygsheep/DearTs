cmake_minimum_required(VERSION 3.20)

# 示例可执行文件
add_executable(demo_imhex_style
    main.cpp
)

# 包含目录
target_include_directories(demo_imhex_style PRIVATE
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
    ${CMAKE_SOURCE_DIR}/third_party/freetype/include
)

# 源文件（新架构实现）
target_sources(demo_imhex_style PRIVATE
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
    ${CMAKE_SOURCE_DIR}/core/ui/command_palette.cpp
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
target_link_libraries(demo_imhex_style PRIVATE
        imgui
        imgui_sdl3
        SDL3-static
        deartsdl_logger
        freetype
        lunasvg::lunasvg
)

# Windows特定设置
if(WIN32)
    target_compile_definitions(demo_imhex_style PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
    )
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
add_custom_command(TARGET demo_imhex_style POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${RESOURCE_SOURCE_DIR}/fonts
        ${RESOURCE_DEST_DIR}/fonts
    COMMENT "Copying font files to output directory"
)

# 复制其他资源文件（如果有的话）
if(EXISTS ${RESOURCE_SOURCE_DIR}/images)
    add_custom_command(TARGET demo_imhex_style POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}/images
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${RESOURCE_SOURCE_DIR}/images
            ${RESOURCE_DEST_DIR}/images
        COMMENT "Copying image files to output directory"
    )
endif()

# 复制配置文件（如果有的话）
if(EXISTS ${RESOURCE_SOURCE_DIR}/config)
    add_custom_command(TARGET demo_imhex_style POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}/config
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${RESOURCE_SOURCE_DIR}/config
            ${RESOURCE_DEST_DIR}/config
        COMMENT "Copying config files to output directory"
    )
endif()

message(STATUS "Demo application configured: demo_imhex_style")
message(STATUS "Resource files will be copied to: ${RESOURCE_DEST_DIR}")
