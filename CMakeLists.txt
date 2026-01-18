cmake_minimum_required(VERSION 3.20)
project(DearTs VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# 禁用 vcpkg manifest 模式（使用已手动安装的 sqlite3）
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

message("vcpkg Path: $ENV{VCPKG_ROOT}")
# ==================== CCache 配置 ====================
# CCache 是一个编译器缓存工具，可以显著加快重复编译速度
#
# 效果：
#   - 首次编译：正常时间（略慢约 5%）
#   - 重复编译：快 90%+（命中缓存）
#   - 切换分支后重新编译：快 80%+
#
# Windows 安装：
#   1. 下载：https://github.com/ccache/ccache/releases
#   2. 解压到：C:\Program Files\ccache
#   3. 添加到 PATH：C:\Program Files\ccache
#   4. 验证：ccache --version
#
# Linux 安装：
#   sudo apt install ccache  # Ubuntu/Debian
#   sudo yum install ccache  # CentOS/RHEL
#
# 启用方法：
#   - 自动：如果系统安装了 ccache，CMake 会自动使用
#   - 手动：设置环境变量 CC=ccache CXX=ccache
#   - 强制禁用：-DDEARTS_ENABLE_CCACHE=OFF
# ==============================================================

option(DEARTS_ENABLE_CCACHE "Enable ccache for faster rebuilds" ON)

if(DEARTS_ENABLE_CCACHE)
    # 查找 ccache 程序
    find_program(CCACHE_PROGRAM ccache)

    if(CCACHE_PROGRAM)
        message(STATUS "CCache found: ${CCACHE_PROGRAM}")
        message(STATUS "CCache version: ${CCACHE_PROGRAM}")

        # 设置 CMake 使用 ccache
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})

        # 配置 ccache 缓存大小（默认 5GB，可调整）
        set(CCACHE_MAXSIZE "5G" CACHE STRING "CCache maximum cache size")
        set(ENV{CCACHE_MAXSIZE} ${CCACHE_MAXSIZE})

        # 配置 ccache 缓存目录
        if(NOT DEFINED CCACHE_DIR)
            if(WIN32)
                set(CCACHE_DIR "${CMAKE_SOURCE_DIR}/.ccache")
            else()
                set(CCACHE_DIR "$HOME/.ccache")
            endif()
        endif()
        set(ENV{CCACHE_DIR} ${CCACHE_DIR})

        message(STATUS "CCache configuration:")
        message(STATUS "  Cache dir: ${CCACHE_DIR}")
        message(STATUS "  Max size: ${CCACHE_MAXSIZE}")
    else()
        message(WARNING "CCache not found. Install ccache for faster rebuilds.")
        message(STATUS "  Windows: https://github.com/ccache/ccache/releases")
        message(STATUS "  Linux: sudo apt install ccache")
    endif()
else()
    message(STATUS "CCache disabled by DEARTS_ENABLE_CCACHE=OFF")
endif()

# CCache 配置结束

# ==================== FFmpeg 配置 ====================
# FFmpeg 是一个强大的音视频处理库
#
# 安装方法：
#   Windows:
#     1. 安装 vcpkg: https://github.com/Microsoft/vcpkg
#     2. 运行: vcpkg install ffmpeg:x64-windows
#     3. 设置环境变量: VCPKG_ROOT=<vcpkg 路径>
#   Linux:
#     1. 安装 vcpkg
#     2. 运行: vcpkg install ffmpeg:x64-linux
#
# CMake 集成：
#   - vcpkg 会自动提供 FFmpeg CMake 目标
#   - 使用: target_link_libraries(${TARGET} PRIVATE FFmpeg::avcodec FFmpeg::avformat ...)
#   - 定义: DEARTS_FFMPEG_SUPPORT 宏用于条件编译
#
# 依赖库：
#   - FFmpeg::avcodec    - 编解码器库
#   - FFmpeg::avformat   - 容器格式库
#   - FFmpeg::avutil     - 工具库
#   - FFmpeg::swscale    - 图像缩放库
#   - FFmpeg::swresample - 音频重采样库
#
# 禁用方法：
#   - 设置: -DENABLE_FFMPEG=OFF
# ==============================================================

option(ENABLE_FFMPEG "Enable FFmpeg support for video/audio processing" ON)

if(ENABLE_FFMPEG)
    message(STATUS "Looking for FFmpeg...")

    # 查找 FFmpeg（vcpkg 提供的 FindFFMPEG.cmake）
    find_package(FFMPEG QUIET)

    # vcpkg 的 FFmpeg 包提供 FFMPEG_FOUND 变量而非 CMake 目标
    if(FFMPEG_FOUND)
        message(STATUS "FFmpeg: Found (v${FFMPEG_VERSION})")

        # 显示找到的库
        if(DEFINED FFMPEG_LIBRARIES)
            message(STATUS "  Libraries: ${FFMPEG_LIBRARIES}")
        endif()

        # 显示头文件路径
        if(DEFINED FFMPEG_INCLUDE_DIRS)
            message(STATUS "  Include dirs: ${FFMPEG_INCLUDE_DIRS}")
        endif()

        # 创建 FFmpeg 接口库
        add_library(ffmpeg_interface INTERFACE)

        # 添加头文件目录
        target_include_directories(ffmpeg_interface INTERFACE ${FFMPEG_INCLUDE_DIRS})

        # 链接 FFmpeg 库
        target_link_libraries(ffmpeg_interface INTERFACE ${FFMPEG_LIBRARIES})

        # 添加 FFmpeg 编译定义
        target_compile_definitions(ffmpeg_interface INTERFACE
            DEARTS_FFMPEG_SUPPORT=1
        )

        message(STATUS "FFmpeg: Enabled")
    else()
        message(WARNING "FFmpeg not found. Video/audio features will be disabled.")
        message(STATUS "  To enable FFmpeg:")
        message(STATUS "    1. Install vcpkg: https://github.com/Microsoft/vcpkg")
        message(STATUS "    2. Run: vcpkg install ffmpeg:x64-windows")
        message(STATUS "    3. Set VCPKG_ROOT environment variable")
        message(STATUS "    4. Use CMake preset: cmake --preset default")

        # 创建空的接口库
        add_library(ffmpeg_interface INTERFACE)
        target_compile_definitions(ffmpeg_interface INTERFACE
            DEARTS_FFMPEG_SUPPORT=0
        )
    endif()
else()
    message(STATUS "FFmpeg: Disabled by ENABLE_FFMPEG=OFF")
    add_library(ffmpeg_interface INTERFACE)
    target_compile_definitions(ffmpeg_interface INTERFACE
        DEARTS_FFMPEG_SUPPORT=0
    )
endif()

# FFmpeg 配置结束
# ==============================================================

# MSVC: Enable /Zc:preprocessor for __VA_OPT__ support (C++20)
if(MSVC)
    add_compile_options(/Zc:preprocessor)
endif()

# Options
option(BUILD_UI_TESTS "Build UI tests with ImGui Test Engine" OFF)
option(ENABLE_IMGUI_TEST_ENGINE "Enable ImGui Test Engine hooks in ImGui" ${BUILD_UI_TESTS})
option(BUILD_EXAMPLES "Build example programs" OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add SDL3 as subdirectory
set(SDL_BUILD_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)
set(SDL_BUILD_EXAMPLES OFF CACHE BOOL "Build SDL examples" FORCE)
set(SDL_DISABLE_INSTALL OFF CACHE BOOL "Disable SDL installation" FORCE)
set(SDL_SHARED ON CACHE BOOL "Build SDL as shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL as static library" FORCE)
set(SDL_INSTALL_CMAKEDIR_ARCH_INDEPENDENT ON CACHE BOOL "Install CMake files arch-independent" FORCE)
set(SDL_INSTALL_TESTS OFF CACHE BOOL "Install test binaries" FORCE)
set(SDL_INSTALL_EXAMPLES OFF CACHE BOOL "Install example binaries" FORCE)

# Add FreeType as subdirectory
set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable Brotli support" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "Disable Bzip2 support" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable HarfBuzz support" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "Disable PNG support" FORCE)
set(FT_DISABLE_ZLIB ON CACHE BOOL "Disable Zlib support" FORCE)
set(FT_ENABLE_DEBUG_LOGGING OFF CACHE BOOL "Disable debug logging" FORCE)

# Add nlohmann/json as subdirectory
set(JSON_BuildTests OFF CACHE BOOL "Build nlohmann/json tests" FORCE)
set(JSON_CI OFF CACHE BOOL "Enable CI build targets" FORCE)
set(JSON_MultipleHeaders ON CACHE BOOL "Use multi-header version" FORCE)

add_subdirectory(third_party/SDL)

# 使用预编译的 SDL_image 库（third_party/SDL_image 已编译）
add_library(SDL3_image-static STATIC IMPORTED)
set_target_properties(SDL3_image-static PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/SDL_image/lib/x64/SDL3_image.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/SDL_image/include/SDL3_image"
)

add_subdirectory(third_party/freetype)
add_subdirectory(third_party/lunasvg)
add_subdirectory(third_party/json)

# Add DearTs Logger library
add_subdirectory(lib/liblogger)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/core)
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/third_party/SDL/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/freetype/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/lunasvg/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/json/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/imgui)
include_directories(${CMAKE_SOURCE_DIR}/third_party/imgui/backends)
include_directories(${CMAKE_SOURCE_DIR}/third_party/imgui_markdown)  # Markdown rendering support

# Setup ImGui (use third_party version)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
if(EXISTS ${IMGUI_DIR})
    message(STATUS "Using ImGui from third_party directory")

    # ImGui core sources
    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
    )

    # SDL3 backend sources
    set(IMGUI_SDL3_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
    )

    # Create ImGui core library
    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/misc/freetype
        ${CMAKE_SOURCE_DIR}/third_party/freetype/include
        ${CMAKE_SOURCE_DIR}/third_party/lunasvg/include
        ${CMAKE_SOURCE_DIR}/core/config  # ImGui user config
    )
    # Link FreeType and LunaSVG (required for imgui_freetype.cpp)
    target_link_libraries(imgui PUBLIC freetype lunasvg::lunasvg)
    # Add Test Engine include path if available (for imgui_te_imconfig.h)
    if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine/imgui_test_engine)
        target_include_directories(imgui PUBLIC
            ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine/imgui_test_engine
        )
    endif()
    # ImGui 编译定义
    target_compile_definitions(imgui PUBLIC
        IMGUI_USER_CONFIG="imgui_user_config.h"
        # 重要：定义 LUNASVG_BUILD_STATIC 解决链接错误
        # 原理：
        #   1. LunaSVG 使用 Windows DLL 导出/导入宏（__declspec(dllexport/dllimport)）
        #   2. 当编译为静态库时，必须定义 LUNASVG_BUILD_STATIC，否则：
        #      - LunaSVG 头文件会将符号声明为 __declspec(dllimport)
        #      - 链接器会尝试从 DLL 导入符号（产生 LNK2019 错误）
        #      - 但实际上我们链接的是静态库 lunasvg.lib，导致链接失败
        #   3. 定义 LUNASVG_BUILD_STATIC 后：
        #      - LunaSVG 头文件不会添加任何导入/导出修饰符
        #      - 符号直接链接到静态库，链接成功
        LUNASVG_BUILD_STATIC
    )
    # 只在启用 Test Engine 时定义 IMGUI_ENABLE_TEST_ENGINE
    if(ENABLE_IMGUI_TEST_ENGINE)
        # 重要：定义 IMGUI_ENABLE_TEST_ENGINE 启用 ImGui Test Engine 支持
        # 原理：
        #   1. ImGui Test Engine 需要访问 ImGuiItemStatusFlags_* 等内部标志
        #   2. 这些标志只在 IMGUI_ENABLE_TEST_ENGINE 定义时才会编译到 imgui 中
        #   3. 不定义此宏会导致：ImGuiItemStatusFlags_Openable/Opened/Checkable/Checked/Inputable 等未声明
        #   4. 定义后：ImGui Test Engine 可以正确访问这些内部状态标志进行 UI 自动化测试
        target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_TEST_ENGINE)
    endif()
    if(WIN32)
        target_compile_definitions(imgui PUBLIC _USE_MATH_DEFINES)
    endif()

    # Create ImGui SDL3 backend library
    add_library(imgui_sdl3 STATIC ${IMGUI_SDL3_SOURCES})
    target_include_directories(imgui_sdl3 PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )
    target_link_libraries(imgui_sdl3 PRIVATE imgui)

    # Create an alias library that includes ImGui + Test Engine (if enabled)
    # This ensures all targets get the Test Engine linkage when needed
    add_library(imgui_full INTERFACE)
    target_link_libraries(imgui_full INTERFACE imgui imgui_sdl3)
    if(ENABLE_IMGUI_TEST_ENGINE)
        if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine/imgui_test_engine)
            target_link_libraries(imgui_full INTERFACE imgui_test_engine)
        endif()
    endif()

    # Setup ImPlot
    set(IMPLOT_DIR ${CMAKE_SOURCE_DIR}/third_party/implot)
    if(EXISTS ${IMPLOT_DIR})
        message(STATUS "Using ImPlot from third_party directory")

        # ImPlot sources
        set(IMPLOT_SOURCES
            ${IMPLOT_DIR}/implot.cpp
            ${IMPLOT_DIR}/implot_items.cpp
        )

        # Create ImPlot library
        add_library(implot STATIC ${IMPLOT_SOURCES})
        target_include_directories(implot PUBLIC
            ${IMPLOT_DIR}
            ${IMGUI_DIR}
        )
        target_link_libraries(implot PUBLIC imgui)

        message(STATUS "ImPlot: Static library")
    else()
        message(WARNING "ImPlot not found in third_party/implot directory!")
    endif()


else()
    message(FATAL_ERROR "ImGui not found in third_party/imgui directory!")
endif()

# Setup ImGui Test Engine (compatible with ImGui v1.92.5)
# Only build if explicitly enabled
if(ENABLE_IMGUI_TEST_ENGINE)
    set(IMGUI_TEST_ENGINE_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine/imgui_test_engine)
    if(EXISTS ${IMGUI_TEST_ENGINE_DIR})
        message(STATUS "Using ImGui Test Engine from third_party directory")

        # ImGui Test Engine sources
        file(GLOB IMGUI_TEST_ENGINE_SOURCES
            ${IMGUI_TEST_ENGINE_DIR}/*.cpp
        )

        # Create ImGui Test Engine library
        add_library(imgui_test_engine STATIC ${IMGUI_TEST_ENGINE_SOURCES})
        target_include_directories(imgui_test_engine PUBLIC
            ${IMGUI_TEST_ENGINE_DIR}
            ${IMGUI_DIR}
            ${CMAKE_SOURCE_DIR}/third_party/imgui
        )
        target_link_libraries(imgui_test_engine PUBLIC imgui)
        target_compile_definitions(imgui_test_engine PUBLIC
            IMGUI_TEST_ENGINE_ENABLE
            IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL
        )
        message(STATUS "ImGui Test Engine: Static library (v1.92.5 compatible)")
    else()
        message(WARNING "ImGui Test Engine requested but not found!")
    endif()
else()
    message(STATUS "ImGui Test Engine: Disabled (use -DENABLE_IMGUI_TEST_ENGINE=ON to enable)")
endif()

# Add GUI test application
add_subdirectory(main/gui)
add_subdirectory(main/chatmanager)

# Add test plugin (for auto-discovery testing)
add_subdirectory(plugins/test_plugin)
add_subdirectory(plugins/test_dependency_plugin)

# ==================== 预编译头 (PCH) 配置 ====================
# 预编译头配置模块位于 cmake/PCH.cmake
#
# 使用方法（在 main/gui/CMakeLists.txt 中）：
#   # 在创建目标后启用 PCH
#   add_executable(${PROJECT_NAME} ...)
#   include(cmake/PCH.cmake)
#   dearts_enable_pch(${PROJECT_NAME})
#
# 注意事项：
#   1. 预编译头可以显著加快编译速度（首次编译后，后续编译更快）
#   2. 修改 pch.hpp 后需要重新编译整个项目
#   3. pch.hpp 中应包含常用但不常修改的头文件
#   4. 当前项目未启用 PCH，需要时可取消注释以下代码启用：
#
# # 创建 pch.cpp（如果不存在）
# if(NOT EXISTS "${CMAKE_SOURCE_DIR}/core/pch.cpp")
#     file(WRITE "${CMAKE_SOURCE_DIR}/core/pch.cpp"
#         "// DearTs Framework - Precompiled Header Source\n"
#         "#include \"core/pch.hpp\"\n"
#     )
# endif()
#
# # 启用预编译头
# option(DEARTS_ENABLE_PCH "Enable precompiled headers" ON)
# if(DEARTS_ENABLE_PCH)
#     include(cmake/PCH.cmake)
#     # 在 add_executable 后调用：
#     # dearts_enable_pch(DearTs)
# endif()
# ==============================================================

# Add examples (only if BUILD_EXAMPLES is ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples/demo_imhex_style)
    add_subdirectory(examples/toast_example)
    add_subdirectory(examples/markdown_example)
    add_subdirectory(examples/sdl_render_example)
    add_subdirectory(examples/sdl_interactive_example)
endif()

# Add plugins
# Note: builtin plugin is compiled directly into main executable, not as separate library
# add_subdirectory(plugins/builtin)
add_subdirectory(plugins/memory_core)    # Memory Core: 持久化和 RAG 服务
add_subdirectory(plugins/chat)
add_subdirectory(plugins/toast_notification)
add_subdirectory(plugins/command_palette)
add_subdirectory(plugins/clipboard_parser)  # 剪切板智能解析插件
add_subdirectory(plugins/live2d)
add_subdirectory(plugins/ffmpeg)  # FFmpeg 视频处理插件

# Add tests (optional, can be enabled with -DBUILD_TESTING=ON)
option(BUILD_TESTING "Build the testing tree" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "Final Working Configuration:")
message(STATUS "  SDL3: Static library")
message(STATUS "  FreeType: Version 2.13.3 (Static)")
message(STATUS "  nlohmann/json: Version 3.12.0 (Header-only)")
message(STATUS "  Core: Source code (not a library)")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Framework: SDL3 + Application Lifecycle")
