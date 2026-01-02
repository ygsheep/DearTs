cmake_minimum_required(VERSION 3.20)

# Live2D 示例程序
project(live2d_example)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==================== Live2D SDK 配置 ====================
set(LIVE2D_SDK_DIR ${CMAKE_SOURCE_DIR}/third_party/Live2D)
set(LIVE2D_CORE_DIR ${LIVE2D_SDK_DIR}/Core)
set(LIVE2D_FRAMEWORK_DIR ${LIVE2D_SDK_DIR}/Framework)

# ==================== 源文件 ====================
set(EXAMPLE_SOURCES
    main.cpp
    opengl_application.cpp
    ${CMAKE_SOURCE_DIR}/third_party/glew/src/glew.c  # GLEW 源文件
)

# ==================== 可执行文件 ====================
add_executable(${PROJECT_NAME} ${EXAMPLE_SOURCES})

# ==================== 包含目录 ====================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib  # 用于 liblogger/logger.h
    ${CMAKE_SOURCE_DIR}/third_party/SDL/include
    ${CMAKE_SOURCE_DIR}/third_party/SDL_image/include  # SDL3_image
    ${CMAKE_SOURCE_DIR}/third_party/imgui
    ${CMAKE_SOURCE_DIR}/third_party/imgui/backends
    ${CMAKE_SOURCE_DIR}/third_party/glew/include  # GLEW
    ${CMAKE_SOURCE_DIR}/third_party/json/include  # nlohmann/json
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/plugins/live2d/include
    ${CMAKE_SOURCE_DIR}/plugins/live2d/include/widgets
    ${CMAKE_SOURCE_DIR}/plugins/live2d/include/views
    ${CMAKE_SOURCE_DIR}/plugins/live2d/renderer
    ${CMAKE_SOURCE_DIR}/plugins/settings/include
    ${LIVE2D_FRAMEWORK_DIR}/src  # Live2D Framework src 目录（用于相对包含路径）
)

# Windows 平台特定设置
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # Windows x64
        if(MSVC_VERSION EQUAL 1930)
            set(LIVE2D_VS_TOOLSET "143")
        elseif(MSVC_VERSION EQUAL 1920)
            set(LIVE2D_VS_TOOLSET "142")
        elseif(MSVC_VERSION EQUAL 1910)
            set(LIVE2D_VS_TOOLSET "141")
        else()
            set(LIVE2D_VS_TOOLSET "140")
        endif()
        set(LIVE2D_CORE_LIB_DIR ${LIVE2D_CORE_DIR}/lib/windows/x86_64/${LIVE2D_VS_TOOLSET})
        set(LIVE2D_CORE_DLL_DIR ${LIVE2D_CORE_DIR}/dll/windows/x86_64)
    endif()

    # Live2D Core 导入库
    add_library(Live2DCubismCore STATIC IMPORTED)
    set_target_properties(Live2DCubismCore PROPERTIES
        IMPORTED_LOCATION ${LIVE2D_CORE_DLL_DIR}/Live2DCubismCore.lib
        INTERFACE_INCLUDE_DIRECTORIES ${LIVE2D_CORE_DIR}/include
    )
endif()

# 收集 Live2D Framework 源文件
file(GLOB_RECURSE LIVE2D_FRAMEWORK_SOURCES
    ${LIVE2D_FRAMEWORK_DIR}/src/*.cpp
)
list(FILTER LIVE2D_FRAMEWORK_SOURCES EXCLUDE REGEX ".*D3D.*")
list(FILTER LIVE2D_FRAMEWORK_SOURCES EXCLUDE REGEX ".*Metal.*")
list(FILTER LIVE2D_FRAMEWORK_SOURCES EXCLUDE REGEX ".*Vulkan.*")

# Live2D 插件源文件
set(LIVE2D_PLUGIN_SOURCES
    ${CMAKE_SOURCE_DIR}/plugins/live2d/source/live2d_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/live2d/source/live2d_model_instance.cpp
    ${CMAKE_SOURCE_DIR}/plugins/live2d/source/live2d_model_manager.cpp
    ${CMAKE_SOURCE_DIR}/plugins/live2d/source/widgets/live2d_settings_widget.cpp
    ${CMAKE_SOURCE_DIR}/plugins/live2d/source/views/live2d_settings_view.cpp
    ${CMAKE_SOURCE_DIR}/plugins/live2d/renderer/live2d_renderer_gl.cpp
    ${CMAKE_SOURCE_DIR}/plugins/live2d/renderer/sdl_texture_uploader.cpp
    # Core dependencies
    ${CMAKE_SOURCE_DIR}/core/content/settings.cpp
    ${CMAKE_SOURCE_DIR}/core/content/commands.cpp
    ${CMAKE_SOURCE_DIR}/core/content/tools.cpp
    ${CMAKE_SOURCE_DIR}/core/content/callbacks.cpp
    ${CMAKE_SOURCE_DIR}/core/content/project_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/config/config_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/plugin.cpp
    ${CMAKE_SOURCE_DIR}/core/tasks/task_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/view.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/view_manager.cpp
    # ImGui 后端（SDL3 + OpenGL3）
    ${CMAKE_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_sdl3.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_opengl3.cpp
)

# 添加所有源文件
target_sources(${PROJECT_NAME} PRIVATE
    ${EXAMPLE_SOURCES}
    ${LIVE2D_FRAMEWORK_SOURCES}
    ${LIVE2D_PLUGIN_SOURCES}
)

# ==================== SDL3_image 导入库配置 ====================
if(WIN32)
    add_library(SDL3_image-static STATIC IMPORTED)
    set_target_properties(SDL3_image-static PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/SDL_image/lib/x64/SDL3_image.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/SDL_image/include"
    )
endif()

# ==================== 链接库 ====================
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui
#    SDL3-static
    SDL3-shared
    SDL3_image-static
    deartsdl_logger
    freetype
    opengl32
    Live2DCubismCore
)

# ==================== 编译定义 ====================
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CSM_TARGET_WIN_GL=1
    LIVE2D_OPENGL=1
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
        GLEW_STATIC
    )
endif()

# ==================== 资源复制 ====================
set(RESOURCE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
set(RESOURCE_DEST_DIR ${CMAKE_BINARY_DIR}/bin/resources)

# 只复制 fonts 目录（live2d 资源需要手动添加）

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

# ==================== 输出目录 ====================
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "Live2D Example configured:")
message(STATUS "  Backend: OpenGL 3.3 Core")
message(STATUS "  Live2D SDK: ${LIVE2D_SDK_DIR}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
