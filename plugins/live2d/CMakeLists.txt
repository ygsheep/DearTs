# Live2D Plugin CMake Configuration

cmake_minimum_required(VERSION 3.20)
project(Live2DPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
#add_subdirectory(${GLEW_DIR}/build/cmake ${CMAKE_BINARY_DIR}/glew)

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
# Note: Keep OpenGL ES2 renderer - it works with desktop OpenGL via GLEW

# Plugin source files
set(PLUGIN_SOURCES
    source/live2d_plugin.cpp
    source/live2d_model_instance.cpp
    source/live2d_model_manager.cpp
    source/widgets/live2d_settings_widget.cpp
    source/views/live2d_settings_view.cpp
    renderer/live2d_renderer_gl.cpp
    renderer/sdl_texture_uploader.cpp
    # Core dependencies (for View, ContentRegistry, etc.)
    ${CMAKE_SOURCE_DIR}/core/config/config_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/content/settings.cpp
    ${CMAKE_SOURCE_DIR}/core/content/commands.cpp
    ${CMAKE_SOURCE_DIR}/core/content/tools.cpp
    ${CMAKE_SOURCE_DIR}/core/content/callbacks.cpp
    ${CMAKE_SOURCE_DIR}/core/content/project_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/view.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/view_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/tasks/task_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/plugin.cpp
)

# Plugin header files
set(PLUGIN_HEADERS
    include/live2d_plugin.hpp
    include/live2d_model_instance.hpp
    include/live2d_model_manager.hpp
    include/widgets/live2d_settings_widget.hpp
    include/views/live2d_settings_view.hpp
    renderer/ilive2d_renderer.hpp
    renderer/live2d_renderer_gl.hpp
    renderer/sdl_texture_uploader.hpp
)

# Create plugin library
add_library(${PROJECT_NAME} SHARED
    ${LIVE2D_FRAMEWORK_SOURCES}
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/lib/liblogger/include
        ${CMAKE_SOURCE_DIR}/third_party/imgui
        ${CMAKE_SOURCE_DIR}/third_party/SDL/include
        ${CMAKE_SOURCE_DIR}/third_party/SDL_image/include
        ${LIVE2D_CORE_DIR}/include
        ${LIVE2D_FRAMEWORK_DIR}/src
        ${GLEW_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/widgets
        ${CMAKE_CURRENT_SOURCE_DIR}/include/views
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer
        ${CMAKE_SOURCE_DIR}/plugins/settings/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL3::SDL3-shared
        SDL3_image-static
        deartsdl_logger
        glew_s
        imgui
)

# Link Live2D Core library
if(WIN32)
    # Windows: Import library for DLL
    # Note: The import lib is in the DLL directory, not the lib directory
    add_library(Live2DCubismCore STATIC IMPORTED)
    set_target_properties(Live2DCubismCore PROPERTIES
        IMPORTED_LOCATION ${LIVE2D_CORE_DLL_DIR}/Live2DCubismCore.lib
        INTERFACE_INCLUDE_DIRECTORIES ${LIVE2D_CORE_DIR}/include
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE Live2DCubismCore)

    # Copy Live2D Core DLL to output directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LIVE2D_CORE_DLL_DIR}/Live2DCubismCore.dll
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying Live2D Core DLL to output directory"
    )
elseif(APPLE OR UNIX)
    # Unix-like: Static library
    add_library(Live2DCubismCore STATIC IMPORTED)
    set_target_properties(Live2DCubismCore PROPERTIES
        IMPORTED_LOCATION ${LIVE2D_CORE_LIB_DIR}/libLive2DCubismCore.a
        INTERFACE_INCLUDE_DIRECTORIES ${LIVE2D_CORE_DIR}/include
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE Live2DCubismCore)
endif()

# Windows specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        BUILDING_DLL
        _USE_MATH_DEFINES
        _CRT_SECURE_NO_WARNINGS
        # 防止 Windows RPC 头文件定义 private/public/protected 宏
        RPC_NO_WINDOWS_H
    )

    # Enable C++20 __VA_OPT__ support
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor)
endif()

# Output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)

# Live2D Framework requires specific definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    # Live2D Framework config
    CSM_TARGET_WIN_GL=1
    # GLEW static library
    GLEW_STATIC
    # Add other platform-specific defines as needed
)

# Install rules
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
)

# Plugin metadata
set(PLUGIN_NAME "Live2D")
set(PLUGIN_AUTHOR "DearTs Team")
set(PLUGIN_DESCRIPTION "Live2D Cubism SDK integration for character rendering")
set(PLUGIN_VERSION "1.0.0")
set(PLUGIN_API_VERSION "1.0.0")

# Generate plugin metadata file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/plugin.json
    @ONLY
)

# Install plugin metadata
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/plugin.json
    DESTINATION plugins
)

# Print configuration summary
message(STATUS "Live2D Plugin Configuration:")
message(STATUS "  Live2D SDK: ${LIVE2D_SDK_DIR}")
message(STATUS "  Live2D Core Library: ${LIVE2D_CORE_LIB_DIR}")
message(STATUS "  Framework Sources: ${LIVE2D_FRAMEWORK_SOURCES_COUNT} files")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
