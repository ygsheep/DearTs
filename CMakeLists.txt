cmake_minimum_required(VERSION 3.20)
project(DearTs VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_UI_TESTS "Build UI tests with ImGui Test Engine" OFF)
option(ENABLE_IMGUI_TEST_ENGINE "Enable ImGui Test Engine hooks in ImGui" ${BUILD_UI_TESTS})
option(BUILD_EXAMPLES "Build example programs" OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 添加自定义 CMake 模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 关键修复：禁用预编译头（必须在添加 SDL 子目录之前设置）
# 设置全局 CMAKE_DISABLE_PRECOMPILE_HEADERS 禁用所有 PCH
set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON CACHE BOOL "Disable all precompiled headers" FORCE)

# Add SDL3 as subdirectory
set(SDL_BUILD_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)
set(SDL_BUILD_EXAMPLES OFF CACHE BOOL "Build SDL examples" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "Disable SDL installation" FORCE)
set(SDL_SHARED OFF CACHE BOOL "Build SDL as shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL as static library" FORCE)
set(SDL sdl3-static)  # 设置 SDL 目标名称
# 关键修复：设置 SDL_REVISION 避免 git 调用，防止 CMake 无限重新配置
set(SDL_REVISION "SDL-3.3.7-prerelease" CACHE STRING "SDL revision (skip git)" FORCE)
# 同样修复 SDL_image
set(SDLIMAGE_REVISION "SDL3_image-3.3.0" CACHE STRING "SDL_image revision (skip git)" FORCE)

# 重要：设置 SDL3 的安装目录，让 SDL_image 能找到它
set(CMAKE_INSTALL_FULL_LIBDIR ${CMAKE_BINARY_DIR}/lib CACHE PATH "")
set(CMAKE_INSTALL_FULL_INCLUDEDIR ${CMAKE_BINARY_DIR}/include CACHE PATH "")

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

# 验证 SDL3 目标已创建
if(TARGET SDL3::Headers AND TARGET SDL3::SDL3)
    message(STATUS "SDL3 targets found: SDL3::Headers and SDL3::SDL3")
else()
    message(FATAL_ERROR "SDL3 targets not found. Check SDL3 configuration.")
endif()

# 添加 SDL_image（它需要找到 SDL3）
# 关键：在添加 SDL_image 之前设置所有必要的变量
# 确保 SDL_image 只构建静态库

# 1. 设置全局 BUILD_SHARED_LIBS 为 OFF
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# 2. 设置 SDL_image 特定选项
set(SDLIMAGE_BUILD_SHARED_LIBS OFF CACHE BOOL "Build SDL3_image as shared library" FORCE)
set(SDLIMAGE_STATIC ON CACHE BOOL "Build SDL3_image as static library" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "Build SDL3_image tests" FORCE)
# 关键修复：禁用 SDL_image 的 git revision 检测
set(SDLIMAGE_INSTALL_MAN OFF CACHE BOOL "Install SDL_image man pages (requires git)" FORCE)

# 3. 关键：由于 SDL_image 会检查是否为根项目，让它认为自己是子目录
set(SDLIMAGE_ROOTPROJECT OFF CACHE BOOL "SDL3_image is root project" FORCE)

# 4. 强制 SDL3_FOUND 为 TRUE，跳过 find_package
set(SDL3_FOUND TRUE CACHE BOOL "SDL3 found" FORCE)
set(SDL3_VERSION "3.3.0" CACHE STRING "SDL3 version" FORCE)

# 5. 禁用需要外部依赖的高级图像格式
set(SDLIMAGE_AVIF OFF CACHE BOOL "Use AVIF" FORCE)
set(SDLIMAGE_JXL OFF CACHE BOOL "Use JXL" FORCE)
set(SDLIMAGE_WEBP OFF CACHE BOOL "Use WebP" FORCE)
set(SDLIMAGE_TIF OFF CACHE BOOL "Use TIFF" FORCE)
set(SDLIMAGE_DAV1D OFF CACHE BOOL "Use dav1d" FORCE)
set(SDLIMAGE_VENDORED OFF CACHE BOOL "Use vendored libraries" FORCE)

# 6. 重要：设置 SDL 为静态，确保 SDL_image 使用正确的 SDL3 目标
set(SDL_STATIC ON CACHE BOOL "SDL is static" FORCE)

add_subdirectory(third_party/SDL_image)
add_subdirectory(third_party/freetype)
add_subdirectory(third_party/lunasvg)
add_subdirectory(third_party/json)

# Add DearTs Logger library
add_subdirectory(lib/liblogger)

# ========================================
# 预编译头（PCH）配置 - 加速编译
# ========================================
option(ENABLE_PCH "Enable precompiled headers for faster compilation" OFF)

if(ENABLE_PCH AND NOT CMAKE_VERSION VERSION_LESS "3.16")
    message(STATUS "Precompiled headers: ENABLED")

    # 创建预编译头目标
    add_library(dearts_pch STATIC)
    target_include_directories(dearts_pch PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/lib
        ${CMAKE_SOURCE_DIR}/third_party/SDL/include
        ${CMAKE_SOURCE_DIR}/third_party/freetype/include
        ${CMAKE_SOURCE_DIR}/third_party/lunasvg/include
        ${CMAKE_SOURCE_DIR}/third_party/json/include
        ${CMAKE_SOURCE_DIR}/third_party/imgui
    )

    # 预编译头源文件（在下一步创建）
    target_sources(dearts_pch PRIVATE "${CMAKE_SOURCE_DIR}/core/pch.cpp")

    if(MSVC)
        target_compile_options(dearts_pch PRIVATE /Yu"pch.hpp")
        target_compile_definitions(dearts_pch PRIVATE _CRT_SECURE_NO_WARNINGS)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(dearts_pch PRIVATE -include pch.hpp)
    endif()

    # 创建预编译头库的别名，方便其他目标链接
    add_library(pch_interface INTERFACE)
    target_link_libraries(pch_interface INTERFACE dearts_pch)

    if(MSVC)
        target_compile_options(pch_interface INTERFACE /FI"pch.hpp" /Yu"pch.hpp")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(pch_interface INTERFACE -include pch.hpp)
    endif()
else()
    message(STATUS "Precompiled headers: DISABLED")
    # 创建空接口，避免代码中需要判断
    add_library(pch_interface INTERFACE)
endif()

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

    # Create an alias library that includes ImGui + Test Engine (if available)
    # This ensures all targets get the Test Engine linkage when needed
    add_library(imgui_full INTERFACE)
    target_link_libraries(imgui_full INTERFACE imgui imgui_sdl3)
    if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine/imgui_test_engine)
        target_link_libraries(imgui_full INTERFACE imgui_test_engine)
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
set(IMGUI_TEST_ENGINE_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui_test_engine/imgui_test_engine)
if(EXISTS ${IMGUI_TEST_ENGINE_DIR})
    message(STATUS "Using ImGui Test Engine from third_party directory")

    # ImGui Test Engine sources (显式列表避免 CMake 无限重新配置)
    set(IMGUI_TEST_ENGINE_SOURCES
        ${IMGUI_TEST_ENGINE_DIR}/imgui_capture_tool.cpp
        ${IMGUI_TEST_ENGINE_DIR}/imgui_te_context.cpp
        ${IMGUI_TEST_ENGINE_DIR}/imgui_te_coroutine.cpp
        ${IMGUI_TEST_ENGINE_DIR}/imgui_te_engine.cpp
        ${IMGUI_TEST_ENGINE_DIR}/imgui_te_exporters.cpp
        ${IMGUI_TEST_ENGINE_DIR}/imgui_te_perftool.cpp
        ${IMGUI_TEST_ENGINE_DIR}/imgui_te_ui.cpp
        ${IMGUI_TEST_ENGINE_DIR}/imgui_te_utils.cpp
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
    message(WARNING "ImGui Test Engine not found!")
endif()

# Add GUI test application
add_subdirectory(main/gui)

# Add examples (only if BUILD_EXAMPLES is ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples/demo_imhex_style)
    add_subdirectory(examples/toast_example)
    add_subdirectory(examples/sdl_render_example)
    add_subdirectory(examples/sdl_interactive_example)
endif()

# Add plugins
# Note: builtin plugin is compiled directly into main executable, not as separate library
# add_subdirectory(plugins/builtin)
add_subdirectory(plugins/toast_notification)
add_subdirectory(plugins/command_palette)

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
