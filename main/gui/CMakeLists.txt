# DearTs 应用程序 CMakeLists.txt
# 基于 DearTs 框架的 GUI 应用

cmake_minimum_required(VERSION 3.20)
project(DearTs)

# Live2D SDK directories
set(LIVE2D_SDK_DIR ${CMAKE_SOURCE_DIR}/third_party/Live2D)
set(LIVE2D_CORE_DIR ${LIVE2D_SDK_DIR}/Core)
set(LIVE2D_FRAMEWORK_DIR ${LIVE2D_SDK_DIR}/Framework)

# GLEW directories (for Live2D SDK OpenGL support)
set(GLEW_DIR ${CMAKE_SOURCE_DIR}/third_party/glew)

# Add GLEW as subdirectory to build from source
# Build static library to avoid DLL dependencies
set(BUILD_UTILS OFF CACHE BOOL "Build GLEW utilities" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared GLEW library" FORCE)
add_subdirectory(${GLEW_DIR}/build/cmake ${CMAKE_BINARY_DIR}/glew)

# Platform-specific Live2D Core library
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # Windows x64
        # Detect MSVC toolset version (140=VS2015, 141=VS2017, 142=VS2019, 143=VS2022)
        if(MSVC_VERSION EQUAL 1930)
            set(LIVE2D_VS_TOOLSET "143")
        elseif(MSVC_VERSION EQUAL 1920)
            set(LIVE2D_VS_TOOLSET "142")
        elseif(MSVC_VERSION EQUAL 1910)
            set(LIVE2D_VS_TOOLSET "141")
        else()
            set(LIVE2D_VS_TOOLSET "140")  # Default to VS2015
        endif()

        set(LIVE2D_CORE_LIB_DIR ${LIVE2D_CORE_DIR}/lib/windows/x86_64/${LIVE2D_VS_TOOLSET})
        set(LIVE2D_CORE_DLL_DIR ${LIVE2D_CORE_DIR}/dll/windows/x86_64)
    else()
        # Windows x86
        set(LIVE2D_CORE_LIB_DIR ${LIVE2D_CORE_DIR}/lib/windows/x86/${LIVE2D_VS_TOOLSET})
        set(LIVE2D_CORE_DLL_DIR ${LIVE2D_CORE_DIR}/dll/windows/x86)
    endif()

    # Select runtime library variant (MD/MT)
    if(MSVC)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(LIVE2D_CORE_LIB_SUFFIX "d")
        else()
            set(LIVE2D_CORE_LIB_SUFFIX "")
        endif()
    endif()
elseif(APPLE)
    # macOS
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set(LIVE2D_CORE_LIB_DIR ${LIVE2D_CORE_DIR}/lib/macos/arm64)
    else()
        set(LIVE2D_CORE_LIB_DIR ${LIVE2D_CORE_DIR}/lib/macos/x86_64)
    endif()
else()
    # Linux
    set(LIVE2D_CORE_LIB_DIR ${LIVE2D_CORE_DIR}/lib/linux/x86_64)
endif()

# Collect Live2D Framework source files
file(GLOB_RECURSE LIVE2D_FRAMEWORK_SOURCES
    ${LIVE2D_FRAMEWORK_DIR}/src/*.cpp
)

# Exclude platform-specific renderers we don't use
list(FILTER LIVE2D_FRAMEWORK_SOURCES EXCLUDE REGEX ".*D3D.*")
list(FILTER LIVE2D_FRAMEWORK_SOURCES EXCLUDE REGEX ".*Metal.*")
list(FILTER LIVE2D_FRAMEWORK_SOURCES EXCLUDE REGEX ".*Vulkan.*")

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
    ${CMAKE_SOURCE_DIR}/plugins/settings/include/widgets/character_settings_widget.hpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/settings_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/settings_view.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/toast_settings_widget.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/toast_settings_widget.hpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/widgets/theme_settings_widget.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/widgets/character_settings_widget.cpp
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
    # Live2D 插件
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/include/live2d_plugin.hpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/include/live2d_model_instance.hpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/include/live2d_model_manager.hpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/include/widgets/live2d_settings_widget.hpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/include/views/live2d_settings_view.hpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/source/live2d_plugin.cpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/source/live2d_model_instance.cpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/source/live2d_model_manager.cpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/source/widgets/live2d_settings_widget.cpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/source/views/live2d_settings_view.cpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/renderer/ilive2d_renderer.hpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/renderer/live2d_renderer_gl.hpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/renderer/live2d_renderer_gl.cpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/renderer/sdl_texture_uploader.hpp
#    ${CMAKE_SOURCE_DIR}/plugins/live2d/renderer/sdl_texture_uploader.cpp
    # FFmpeg Plugin (仅在 FFmpeg 支持时)
    $<$<BOOL:DEARTS_FFMPEG_SUPPORT>:${CMAKE_SOURCE_DIR}/plugins/ffmpeg/include/ffmpeg_plugin.hpp>
    $<$<BOOL:DEARTS_FFMPEG_SUPPORT>:${CMAKE_SOURCE_DIR}/plugins/ffmpeg/include/ffmpeg_view.hpp>
    $<$<BOOL:DEARTS_FFMPEG_SUPPORT>:${CMAKE_SOURCE_DIR}/plugins/ffmpeg/source/ffmpeg_plugin.cpp>
    $<$<BOOL:DEARTS_FFMPEG_SUPPORT>:${CMAKE_SOURCE_DIR}/plugins/ffmpeg/source/ffmpeg_view.cpp>
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
    ${CMAKE_SOURCE_DIR}/plugins/live2d/include
    ${CMAKE_SOURCE_DIR}/plugins/live2d/include/widgets
    ${CMAKE_SOURCE_DIR}/plugins/live2d/include/views
    ${CMAKE_SOURCE_DIR}/plugins/live2d/renderer
    ${CMAKE_SOURCE_DIR}/plugins/settings/include
    # FFmpeg Plugin 包含目录
    ${CMAKE_SOURCE_DIR}/plugins/ffmpeg/include
    ${CMAKE_SOURCE_DIR}/third_party/SDL/include
    ${CMAKE_SOURCE_DIR}/third_party/SDL_image/include
    ${CMAKE_SOURCE_DIR}/third_party/freetype/include
    ${CMAKE_SOURCE_DIR}/third_party/imgui
    ${CMAKE_SOURCE_DIR}/third_party/imgui/backends
    ${CMAKE_SOURCE_DIR}/third_party/imgui/misc/freetype
    ${CMAKE_SOURCE_DIR}/third_party/implot
    ${LIVE2D_CORE_DIR}/include
    ${LIVE2D_FRAMEWORK_DIR}/src
    ${GLEW_DIR}/include
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
    ${CMAKE_SOURCE_DIR}/core/ui/background_renderer.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/character_renderer.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/character_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/tasks/task_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/app/application.cpp
    ${CMAKE_SOURCE_DIR}/core/utils/file_dialog.cpp
    # Live2D Framework source files
    ${LIVE2D_FRAMEWORK_SOURCES}
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
        imgui_full
        implot
        SDL3-shared
        SDL3_image-static
        deartsdl_logger
        freetype
        lunasvg::lunasvg
        glew_s
        # FFmpeg (如果可用)
        $<$<BOOL:DEARTS_FFMPEG_SUPPORT>:ffmpeg_interface>
)

# Windows 特定设置
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
    )
    # Release 版本设置为 GUI 应用程序（无控制台窗口）
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            "/SUBSYSTEM:WINDOWS"
        )
    endif()

    # Link Live2D Core library (Windows)
    add_library(Live2DCubismCore STATIC IMPORTED)
    set_target_properties(Live2DCubismCore PROPERTIES
        IMPORTED_LOCATION ${LIVE2D_CORE_DLL_DIR}/Live2DCubismCore.lib
        INTERFACE_INCLUDE_DIRECTORIES ${LIVE2D_CORE_DIR}/include
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE Live2DCubismCore)
endif()

# Live2D Framework requires specific definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    # Live2D Framework config
    CSM_TARGET_WIN_GL=1
    # GLEW static library
    GLEW_STATIC
)

# 定义资源目录
set(RESOURCE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
set(RESOURCE_DEST_DIR ${CMAKE_BINARY_DIR}/bin/resources)

# 复制所有资源文件到输出目录（仅在文件真正变化时复制）
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # 使用 copy_if_different 避免不必要的时间戳更新
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}/fonts
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}/images
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}/live2d
    # 复制字体文件（仅在变化时）
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${RESOURCE_SOURCE_DIR}/fonts
        ${RESOURCE_DEST_DIR}/fonts
    # 复制图片文件（仅在变化时）
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${RESOURCE_SOURCE_DIR}/images
        ${RESOURCE_DEST_DIR}/images
    # 复制 Live2D 模型文件（仅在变化时）
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${RESOURCE_SOURCE_DIR}/models
        ${CMAKE_BINARY_DIR}/bin/models
    # 复制图标文件（仅在变化时）
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#        ${RESOURCE_SOURCE_DIR}/icon.ico
#        ${CMAKE_BINARY_DIR}/bin/icon.ico
    # 复制 SDL3_image.dll（根据架构选择）
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/third_party/SDL_image/lib/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/SDL3_image.dll
        ${CMAKE_BINARY_DIR}/bin/SDL3_image.dll
    # 复制 Live2D Core DLL（仅 Windows）
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${LIVE2D_CORE_DLL_DIR}/Live2DCubismCore.dll
        ${CMAKE_BINARY_DIR}/bin/Live2DCubismCore.dll
    COMMENT "Copying resources to output directory (if changed)"
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
