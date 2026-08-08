cmake_minimum_required(VERSION 3.20)

# 示例可执行文件
add_executable(demo_imhex_style
    main.cpp
)

# 包含目录
target_include_directories(demo_imhex_style PRIVATE
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
    ${DEARTS_ROOT}/third_party/freetype/include
)

# 源文件（新架构实现）
target_sources(demo_imhex_style PRIVATE
    ${DEARTS_ROOT}/core/content/settings.cpp
    ${DEARTS_ROOT}/core/content/commands.cpp
    ${DEARTS_ROOT}/core/content/tools.cpp
    ${DEARTS_ROOT}/core/content/callbacks.cpp
    ${DEARTS_ROOT}/core/content/project_manager.cpp
    ${DEARTS_ROOT}/core/config/config_manager.cpp
    ${DEARTS_ROOT}/core/plugin/plugin.cpp
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
target_link_libraries(demo_imhex_style PRIVATE
        imgui_full
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
    ${DEARTS_ROOT}/third_party/imgui/imconfig.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui_user_config.h
    COPYONLY
)

# ================ 复制资源文件 ================

# 定义资源目录
set(RESOURCE_SOURCE_DIR ${DEARTS_ROOT}/resources)
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
