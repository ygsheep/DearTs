cmake_minimum_required(VERSION 3.20)
project(DearTs VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# ==================== Boost.Asio 配置 ====================
# Boost.Asio 是一个跨平台的 C++ 网络库
#
# 安装方法：
#   Windows:
#     1. 安装 vcpkg: https://github.com/Microsoft/vcpkg
#     2. 运行: vcpkg install boost-asio:x64-windows
#     3. 运行: vcpkg install boost-beast:x64-windows
#     4. 设置环境变量: VCPKG_ROOT=<vcpkg 路径>
#   Linux:
#     1. 安装 vcpkg
#     2. 运行: vcpkg install boost-asio:x64-linux
#     3. 运行: vcpkg install boost-beast:x64-linux
#
# CMake 集成：
#   - vcpkg 会自动提供 Boost CMake 目标
#   - 使用: target_link_libraries(${TARGET} PRIVATE Boost::asio)
#   - 定义: DEARTS_BOOST_ASIO_SUPPORT 宏用于条件编译
#
# 依赖库：
#   - Boost::asio       - 核心 I/O 库（header-only）
#   - Boost::beast      - HTTP/HTTPS 库（header-only）
#   - Boost::system     - 系统库（如果需要）
#
# 禁用方法：
#   - 不适用，Boost.Asio 是必需依赖
# ==============================================================

option(ENABLE_BOOST_ASIO "Enable Boost.Asio support for HTTP requests" ON)

if(ENABLE_BOOST_ASIO)
    message(STATUS "Looking for Boost...")

    # 根据平台使用不同的查找方式
    # Windows (vcpkg): 使用组件查找，vcpkg 会提供 Boost::asio 和 Boost::beast 目标
    # Linux (系统包/Nix): asio 和 beast 是 header-only，使用简单查找
    if(WIN32)
        # Windows: 使用 vcpkg 组件查找
        find_package(Boost REQUIRED COMPONENTS asio beast)
    else()
        # Linux: 简单查找，asio/beast 是 header-only
        find_package(Boost REQUIRED)
    endif()

    if(Boost_FOUND)
        message(STATUS "Boost: Found (version ${Boost_VERSION})")
        message(STATUS "  Include dirs: ${Boost_INCLUDE_DIRS}")
        message(STATUS "  Libraries: ${Boost_LIBRARIES}")

        # 创建 Boost.Asio + Boost.Beast 接口库（header-only）
        add_library(boost_asio INTERFACE)

        # 添加头文件目录
        target_include_directories(boost_asio INTERFACE ${Boost_INCLUDE_DIRS})

        # Linux: 检查并手动创建缺失的 Boost::asio 和 Boost::beast 目标
        # 因为它们是 header-only，可能没有独立的 CMake 配置
        if(NOT WIN32)
            if(NOT TARGET Boost::asio)
                message(STATUS "  Creating Boost::asio target (header-only)")
                add_library(Boost::asio INTERFACE IMPORTED)
                set_target_properties(Boost::asio PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}"
                )
            endif()
            
            if(NOT TARGET Boost::beast)
                message(STATUS "  Creating Boost::beast target (header-only)")
                add_library(Boost::beast INTERFACE IMPORTED)
                set_target_properties(Boost::beast PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}"
                )
            endif()
        endif()

        # 链接 Boost 库
        target_link_libraries(boost_asio INTERFACE Boost::asio Boost::beast)

        # 添加编译定义
        target_compile_definitions(boost_asio INTERFACE
            DEARTS_BOOST_ASIO_SUPPORT=1
        )

        # Windows: 定义 _WIN32_WINNT (Boost.Asio 需要)
        if(WIN32)
            target_compile_definitions(boost_asio INTERFACE
                _WIN32_WINNT=0x0601  # Windows 7 或更高
            )
        endif()

        message(STATUS "Boost.Asio + Boost.Beast: Enabled")
    else()
        message(FATAL_ERROR "Boost.Asio not found. Please install:")
        message(STATUS "  Windows: vcpkg install boost-asio:x64-windows boost-beast:x64-windows")
        message(STATUS "  Linux: sudo apt install libboost-all-dev 或 vcpkg install boost-asio:x64-linux")
    endif()
else()
    message(FATAL_ERROR "Boost.Asio is required. Cannot continue without ENABLE_BOOST_ASIO=ON")
endif()

# Boost.Asio 配置结束
# ==============================================================

# MSVC: Enable /Zc:preprocessor for __VA_OPT__ support (C++20)
if(MSVC)
    add_compile_options(/Zc:preprocessor /bigobj)

    # MSVC 2026 预览版与 vcpkg 的 /external 标志有兼容性问题
    # 需要确保标准库头文件路径正确
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.50")
        # 对于 MSVC 19.50 (2026 预览版)，禁用 external 头文件的隔离
        add_compile_options(/experimental:external-)
    endif()
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

# SDL_image: 从源码构建（共享库）
# 设置 SDL_image 编译选项
set(SDLIMAGE_BUILD_SHARED_LIBS ON CACHE BOOL "Build SDL_image as shared library" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "Build SDL_image tests" FORCE)
set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "Use shared dependencies" FORCE)
# 禁用需要 NASM 的 AV1 解码支持
set(SDLIMAGE_AVIF OFF CACHE BOOL "AVIF support" FORCE)
set(SDLIMAGE_JXL OFF CACHE BOOL "JPEG XL support" FORCE)
set(SDLIMAGE_STB OFF CACHE BOOL "Use stb_image" FORCE)
set(SDLIMAGE_WIC ON CACHE BOOL "Use Windows Imaging Component" FORCE)
add_subdirectory(third_party/SDL_image)

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
        $<BUILD_INTERFACE:${IMGUI_DIR}>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${IMGUI_DIR}/misc/freetype>
        $<INSTALL_INTERFACE:include/misc/freetype>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/freetype/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/lunasvg/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/config>  # ImGui user config
    )
    # Link FreeType and LunaSVG (required for imgui_freetype.cpp)
    target_link_libraries(imgui PUBLIC freetype lunasvg::lunasvg)
    # Add Test Engine include path if available (for imgui_te_imconfig.h)
    if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine/imgui_test_engine)
        target_include_directories(imgui PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine/imgui_test_engine>
            $<INSTALL_INTERFACE:include>
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
        $<BUILD_INTERFACE:${IMGUI_DIR}>
        $<BUILD_INTERFACE:${IMGUI_DIR}/backends>
        $<INSTALL_INTERFACE:include>
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
            $<BUILD_INTERFACE:${IMPLOT_DIR}>
            $<BUILD_INTERFACE:${IMGUI_DIR}>
            $<INSTALL_INTERFACE:include>
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

# ==================== DearTs 框架库 (dearts) ====================
# 将框架核心 (core) + 内置插件编译为可复用静态库
# 使用方可通过 add_subdirectory 链接 dearts，或 find_package(dearts) 使用

option(DEARTS_FFMPEG_SUPPORT "Enable FFmpeg plugin support" ON)

add_library(dearts STATIC
    # ---- core ----
    ${CMAKE_SOURCE_DIR}/core/app/application.cpp
    ${CMAKE_SOURCE_DIR}/core/config/config_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/content/callbacks.cpp
    ${CMAKE_SOURCE_DIR}/core/content/commands.cpp
    ${CMAKE_SOURCE_DIR}/core/content/project_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/content/settings.cpp
    ${CMAKE_SOURCE_DIR}/core/content/tools.cpp
    ${CMAKE_SOURCE_DIR}/core/network/http_client.cpp
    ${CMAKE_SOURCE_DIR}/core/network/sse_parser.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/dependency_resolver.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/plugin.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/plugin_dependency.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/plugin_loader.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/version.cpp
    ${CMAKE_SOURCE_DIR}/core/plugin/version_range.cpp
    ${CMAKE_SOURCE_DIR}/core/tasks/task_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/background_renderer.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/character_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/character_renderer.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/icon_font.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/imgui_extensions.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/imgui_layer.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/layout_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/scale_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/shortcut_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/task_widget.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/theme_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/title_bar.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/view.cpp
    ${CMAKE_SOURCE_DIR}/core/ui/view_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/utils/file_dialog.cpp
    # ---- 内置插件 ----
    ${CMAKE_SOURCE_DIR}/plugins/builtin/source/builtin_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/settings_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/settings_view.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/widgets/toast_settings_widget.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/widgets/theme_settings_widget.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/widgets/character_settings_widget.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/widgets/plugin_manager_widget.cpp
    ${CMAKE_SOURCE_DIR}/plugins/settings/source/widgets/dependency_graph_widget.cpp
    ${CMAKE_SOURCE_DIR}/plugins/logger_viewer/source/logger_viewer_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/logger_viewer/source/logger_viewer_view.cpp
    ${CMAKE_SOURCE_DIR}/plugins/navigation/source/navigation_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/navigation/source/sidebar_view.cpp
    ${CMAKE_SOURCE_DIR}/plugins/toast_notification/source/toast_manager.cpp
    ${CMAKE_SOURCE_DIR}/plugins/toast_notification/source/toast_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/command_palette/source/command_palette_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/command_palette/source/command_palette_view.cpp
    ${CMAKE_SOURCE_DIR}/plugins/clipboard_parser/source/clipboard_parser_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/clipboard_parser/source/views/clipboard_parser_view.cpp
    # FFmpeg 插件（仅当 FFmpeg 可用）
    $<$<BOOL:DEARTS_FFMPEG_SUPPORT>:${CMAKE_SOURCE_DIR}/plugins/ffmpeg/source/ffmpeg_plugin.cpp>
    $<$<BOOL:DEARTS_FFMPEG_SUPPORT>:${CMAKE_SOURCE_DIR}/plugins/ffmpeg/source/ffmpeg_view.cpp>
)

# 导出 include 目录（构建树 + 安装树双模式）
target_include_directories(dearts PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/event>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/content>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/ui>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/plugin>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/config>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/tasks>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/utils>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/app>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/network>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib/liblogger>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/plugins/builtin/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/plugins/settings/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/plugins/settings/include/widgets>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/plugins/logger_viewer/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/plugins/navigation/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/plugins/toast_notification/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/plugins/command_palette/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/plugins/clipboard_parser/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/plugins/ffmpeg/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/imgui>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/imgui/backends>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/imgui/misc/freetype>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/imgui_markdown>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/implot>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/SDL/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/SDL_image/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/freetype/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/json/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/lunasvg/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/cppjieba/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/cppjieba/deps/limonp/include>
    $<INSTALL_INTERFACE:include>
)

# 链接第三方库（PUBLIC 传递，使用方只需链接 dearts）
target_link_libraries(dearts PUBLIC
    imgui_full
    implot
    SDL3::SDL3-shared
    SDL3_image::SDL3_image
    deartsdl_logger
    freetype
    lunasvg::lunasvg
    nlohmann_json::nlohmann_json
    boost_asio
    $<$<BOOL:DEARTS_FFMPEG_SUPPORT>:ffmpeg_interface>
)

target_compile_features(dearts PUBLIC cxx_std_20)

if(WIN32)
    target_compile_definitions(dearts PUBLIC
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
        NOMINMAX
    )
endif()

if(MSVC)
    target_compile_options(dearts PRIVATE /W4 /utf-8)
    # 头文件中的 __VA_OPT__ 需要 MSVC 新预处理器，传递给使用方
    target_compile_options(dearts INTERFACE /Zc:preprocessor)
endif()

# 设置输出目录
set_target_properties(dearts PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

message(STATUS "DearTs Framework Library:")
message(STATUS "  Target: dearts (static)")
message(STATUS "  Sources: core + builtin plugins")
message(STATUS "  FFmpeg support: ${DEARTS_FFMPEG_SUPPORT}")

# ==================== 安装与导出配置 ====================
# dearts 库支持两种使用方式：
#   1. add_subdirectory: 直接链接 dearts 目标
#   2. find_package(dearts): 安装后通过 deartsConfig.cmake 使用
# =========================================================

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# 安装 dearts 库并导出目标
install(TARGETS dearts
    EXPORT deartsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 安装第三方依赖库并一并导出（使 deartsTargets.cmake 自包含）
install(TARGETS
    imgui
    imgui_sdl3
    imgui_full
    implot
    deartsdl_logger
    freetype
    nlohmann_json
    boost_asio
    SDL3-shared
    SDL3_Headers
    SDL3_image-static
    EXPORT deartsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(DEARTS_FFMPEG_SUPPORT)
    install(TARGETS ffmpeg_interface
        EXPORT deartsTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

install(EXPORT deartsTargets
    FILE deartsTargets.cmake
    NAMESPACE DearTs::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dearts
)

# 生成并安装 deartsConfig.cmake
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/deartsConfig.cmake.in
    ${CMAKE_BINARY_DIR}/deartsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dearts
)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/deartsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_BINARY_DIR}/deartsConfig.cmake
    ${CMAKE_BINARY_DIR}/deartsConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dearts
)

# ---- 头文件安装 ----
# core 头文件：保留 core/ 前缀路径
install(DIRECTORY ${CMAKE_SOURCE_DIR}/core/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/core
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
# core 各子目录头文件：平铺（兼容 #include "event/event_bus.h" 写法）
install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/core/event
    ${CMAKE_SOURCE_DIR}/core/content
    ${CMAKE_SOURCE_DIR}/core/ui
    ${CMAKE_SOURCE_DIR}/core/plugin
    ${CMAKE_SOURCE_DIR}/core/config
    ${CMAKE_SOURCE_DIR}/core/tasks
    ${CMAKE_SOURCE_DIR}/core/utils
    ${CMAKE_SOURCE_DIR}/core/app
    ${CMAKE_SOURCE_DIR}/core/network
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# liblogger
install(FILES ${CMAKE_SOURCE_DIR}/lib/liblogger/logger.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_SOURCE_DIR}/lib/liblogger/logger.cpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ImGui 用户配置（IMGUI_USER_CONFIG 引用）
install(FILES ${CMAKE_SOURCE_DIR}/core/config/imgui_user_config.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 内置插件头文件
install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/plugins/builtin/include/
    ${CMAKE_SOURCE_DIR}/plugins/settings/include/
    ${CMAKE_SOURCE_DIR}/plugins/logger_viewer/include/
    ${CMAKE_SOURCE_DIR}/plugins/navigation/include/
    ${CMAKE_SOURCE_DIR}/plugins/toast_notification/include/
    ${CMAKE_SOURCE_DIR}/plugins/command_palette/include/
    ${CMAKE_SOURCE_DIR}/plugins/clipboard_parser/include/
    ${CMAKE_SOURCE_DIR}/plugins/ffmpeg/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# ImGui 头文件
install(FILES
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui.h
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_internal.h
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imconfig.h
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imstb_rectpack.h
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imstb_textedit.h
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imstb_truetype.h
    ${CMAKE_SOURCE_DIR}/third_party/imgui_markdown/imgui_markdown.h
    ${CMAKE_SOURCE_DIR}/third_party/implot/implot.h
    ${CMAKE_SOURCE_DIR}/third_party/implot/implot_internal.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/imgui/backends/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/backends
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/imgui/misc/freetype/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/misc/freetype
    FILES_MATCHING PATTERN "*.h"
)

# SDL3 / SDL3_image 头文件
install(DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/SDL/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/SDL_image/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# FreeType / nlohmann-json / LunaSVG / cppjieba 头文件
install(DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/freetype/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/json/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)
# nlohmann_json 目标的 INTERFACE_SOURCES 引用 natvis（相对安装前缀）
if(MSVC)
    install(FILES ${CMAKE_SOURCE_DIR}/third_party/json/nlohmann_json.natvis
        DESTINATION .
    )
endif()
install(DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/lunasvg/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/cppjieba/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/cppjieba/deps/limonp/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# ---- 运行时 DLL 安装 ----
# SDL3（SDL3_image 为静态库，无 DLL）
install(FILES
    ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/SDL3.dll
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# FFmpeg DLLs（如果启用）
if(ENABLE_FFMPEG AND FFMPEG_FOUND)
    install(FILES
        D:/Programs/vcpkg/installed/x64-windows/bin/avcodec-62.dll
        D:/Programs/vcpkg/installed/x64-windows/bin/avformat-62.dll
        D:/Programs/vcpkg/installed/x64-windows/bin/avutil-60.dll
        D:/Programs/vcpkg/installed/x64-windows/bin/swresample-6.dll
        D:/Programs/vcpkg/installed/x64-windows/bin/swscale-9.dll
        D:/Programs/vcpkg/installed/x64-windows/bin/avfilter-11.dll
        D:/Programs/vcpkg/installed/x64-windows/bin/avdevice-62.dll
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        OPTIONAL
    )
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
add_subdirectory(plugins/wuthering_waves)     # 鸣潮抽卡记录插件 (静态库)
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
