cmake_minimum_required(VERSION 3.20)
project(DearTs VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add SDL3 as subdirectory
set(SDL_BUILD_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)
set(SDL_BUILD_EXAMPLES OFF CACHE BOOL "Build SDL examples" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "Disable SDL installation" FORCE)
set(SDL_SHARED OFF CACHE BOOL "Build SDL as shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL as static library" FORCE)

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
        # 重要：定义 IMGUI_ENABLE_TEST_ENGINE 启用 ImGui Test Engine 支持
        # 原理：
        #   1. ImGui Test Engine 需要访问 ImGuiItemStatusFlags_* 等内部标志
        #   2. 这些标志只在 IMGUI_ENABLE_TEST_ENGINE 定义时才会编译到 imgui 中
        #   3. 不定义此宏会导致：ImGuiItemStatusFlags_Openable/Opened/Checkable/Checked/Inputable 等未声明
        #   4. 定义后：ImGui Test Engine 可以正确访问这些内部状态标志进行 UI 自动化测试
        IMGUI_ENABLE_TEST_ENGINE
    )
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
    )
    message(STATUS "ImGui Test Engine: Static library (v1.92.5 compatible)")
else()
    message(WARNING "ImGui Test Engine not found!")
endif()

# Add GUI test application
add_subdirectory(main/gui)
add_subdirectory(examples/demo_imhex_style)
add_subdirectory(examples/toast_example)
add_subdirectory(examples/sdl_render_example)
add_subdirectory(examples/sdl_interactive_example)

# Add plugins
# Note: builtin plugin is compiled directly into main executable, not as separate library
# add_subdirectory(plugins/builtin)
add_subdirectory(plugins/toast_notification)
add_subdirectory(plugins/command_palette)

# Add tests (optional, can be disabled with -DBUILD_TESTING=OFF)
option(BUILD_TESTING "Build the testing tree" ON)
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
