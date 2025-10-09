cmake_minimum_required(VERSION 3.20)
project(DearTs VERSION 1.0.0 LANGUAGES CXX)

# 项目信息
set(DEARTS_VERSION_MAJOR 1)
set(DEARTS_VERSION_MINOR 0)
set(DEARTS_VERSION_PATCH 0)
set(DEARTS_VERSION "${DEARTS_VERSION_MAJOR}.${DEARTS_VERSION_MINOR}.${DEARTS_VERSION_PATCH}")

# C++标准设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 项目选项
option(DEARTS_BUILD_TESTS "Build tests" OFF)
option(DEARTS_BUILD_DOCS "Build documentation" OFF)
option(DEARTS_BUILD_EXAMPLES "Build examples" OFF)
option(DEARTS_ENABLE_LOGGING "Enable logging" ON)
option(DEARTS_ENABLE_PROFILING "Enable profiling" OFF)

# 设置第三方库路径
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/lib/third_party)
set(IMGUI_DIR ${THIRD_PARTY_DIR}/imgui)
set(SDL2_ROOT ${THIRD_PARTY_DIR}/SDL2)
set(SDL2_TTF_ROOT ${THIRD_PARTY_DIR}/SDL2_ttf)
set(SDL2_IMAGE_ROOT ${THIRD_PARTY_DIR}/SDL2_image)
set(SDL2_MIXER_ROOT ${THIRD_PARTY_DIR}/SDL2_mixer)
set(WINTOAST_ROOT ${THIRD_PARTY_DIR}/WinToast)
set(FREETYPE_ROOT ${THIRD_PARTY_DIR}/freetype)

# 根据平台选择库路径
if(CMAKE_SIZEOF_VOID_P EQUAL 8)  # 64位系统
    set(SDL2_LIB_DIR ${SDL2_ROOT}/lib/x64)
    set(SDL2_TTF_LIB_DIR ${SDL2_TTF_ROOT}/lib/x64)
    set(SDL2_IMAGE_LIB_DIR ${SDL2_IMAGE_ROOT}/lib/x64)
    set(SDL2_MIXER_LIB_DIR ${SDL2_MIXER_ROOT}/lib/x64)
    message(STATUS "Using SDL2 x64 libraries")
else()  # 32位系统
    set(SDL2_LIB_DIR ${SDL2_ROOT}/lib/x86)
    set(SDL2_TTF_LIB_DIR ${SDL2_TTF_ROOT}/lib/x86)
    set(SDL2_IMAGE_LIB_DIR ${SDL2_IMAGE_ROOT}/lib/x86)
    set(SDL2_MIXER_LIB_DIR ${SDL2_MIXER_ROOT}/lib/x86)
    message(STATUS "Using SDL2 x86 libraries")
endif()

# 编译器特定设置
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_options(/W3)  # 降低警告级别避免冲突
    # 运行时库设置通过CMAKE_MSVC_RUNTIME_LIBRARY处理
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()


# 全局包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/lib/libdearts/include
    ${CMAKE_SOURCE_DIR}/main/gui/include
    ${CMAKE_SOURCE_DIR}/plugins/builtin
    ${CMAKE_BINARY_DIR}/include
    ${SDL2_ROOT}/include
    ${SDL2_TTF_ROOT}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${WINTOAST_ROOT}
)

# 配置ImGui
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
    ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_compile_features(imgui PUBLIC cxx_std_20)

# 链接FreeType到ImGui（如果启用）
if(USE_MANUAL_FREETYPE)
    target_link_libraries(imgui PRIVATE freetype)
    target_include_directories(imgui PRIVATE ${FREETYPE_ROOT}/include)
    target_compile_definitions(imgui PRIVATE IMGUI_ENABLE_FREETYPE)
    message(STATUS "FreeType linked to ImGui")
endif()

# 配置WinToast库（仅Windows平台）
if(WIN32)
    # WinToast源文件
    set(WINTOAST_SOURCES
        ${WINTOAST_ROOT}/wintoastlib.cpp
    )

    # 创建WinToast静态库
    add_library(wintoast STATIC ${WINTOAST_SOURCES})
    target_include_directories(wintoast PUBLIC ${WINTOAST_ROOT})
    target_compile_features(wintoast PUBLIC cxx_std_20)

    # Windows特定编译定义
    target_compile_definitions(wintoast PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )

    # 链接Windows Runtime库
    target_link_libraries(wintoast PRIVATE
        runtimeobject
        shell32
        ole32
        oleaut32
    )

    message(STATUS "WinToast library configured for Windows notifications")
endif()

# 配置FreeType库
option(BUILD_FREETYPE "Build FreeType library" OFF)
option(USE_MANUAL_FREETYPE "Use manually compiled FreeType library" ON)

if(USE_MANUAL_FREETYPE)
    # 使用手动编译的FreeType库
    message(STATUS "Using manually compiled FreeType library")

    # 创建FreeType导入库目标
    add_library(freetype STATIC IMPORTED)
    set_target_properties(freetype PROPERTIES
        IMPORTED_LOCATION ${FREETYPE_ROOT}/lib/freetype.lib
        INTERFACE_INCLUDE_DIRECTORIES ${FREETYPE_ROOT}/include
    )

    message(STATUS "Manual FreeType library configured")
elseif(BUILD_FREETYPE)
    # 从源码编译FreeType（禁用）
    message(STATUS "Building FreeType from source (disabled)")
endif()

# 添加子目录
add_subdirectory(core)
add_subdirectory(lib/libdearts)
add_subdirectory(main/gui)
add_subdirectory(plugins)
add_subdirectory(examples)

# 主程序源文件
file(GLOB_RECURSE MAIN_SOURCES
    main/*.cpp
    main/*.hpp
)

# Windows资源文件
if(WIN32)
    set(RESOURCE_FILES ${CMAKE_SOURCE_DIR}/resources/dearts.rc)
endif()

# 创建主程序可执行文件
add_executable(${PROJECT_NAME} ${MAIN_SOURCES} ${RESOURCE_FILES})

# 设置目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "DearTs"
    DEBUG_POSTFIX "_d"
)

# 跨平台GUI应用程序设置
# 设置是否创建无控制台窗口应用程序
option(DEARTS_GUI_APP "Create GUI application without console window" ON)

if(DEARTS_GUI_APP)
    if(WIN32)
        # Windows平台：根据构建类型设置是否显示控制台
        set_target_properties(${PROJECT_NAME} PROPERTIES
                WIN32_EXECUTABLE $<IF:$<CONFIG:Debug>,FALSE,TRUE>
        )
    elseif(APPLE)
        # macOS平台：创建应用程序包
        set_target_properties(${PROJECT_NAME} PROPERTIES
                MACOSX_BUNDLE TRUE
                MACOSX_BUNDLE_BUNDLE_NAME "DearTs"
        )
    endif()
endif()

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    libdearts
    DearTsCore
    imgui
)

# 链接FreeType库
if(USE_MANUAL_FREETYPE)
    target_link_libraries(${PROJECT_NAME} PRIVATE freetype)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FREETYPE_ROOT}/include)
    target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_ENABLE_FREETYPE)
    message(STATUS "FreeType library linked to main project")
endif()

# 链接WinToast库（仅Windows平台）
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE wintoast)
endif()

# 链接SDL2库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_LIB_DIR}/SDL2.lib 
    ${SDL2_LIB_DIR}/SDL2main.lib
    ${SDL2_TTF_LIB_DIR}/SDL2_ttf.lib
    ${SDL2_IMAGE_LIB_DIR}/SDL2_image.lib
    ${SDL2_MIXER_LIB_DIR}/SDL2_mixer.lib
)

# 平台特定库链接
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32
        gdi32
        shell32
        ole32
        oleaut32
        imm32
        winmm
        version
        windowsapp  # 添加Windows Runtime API支持
    )
    
    # Windows特定编译定义
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        __MSVCRT_VERSION__=0x0700  # 支持较新的运行时库
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CMAKE_DL_LIBS}
        pthread
    )
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

# 编译定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    DEARTS_VERSION_MAJOR=${DEARTS_VERSION_MAJOR}
    DEARTS_VERSION_MINOR=${DEARTS_VERSION_MINOR}
    DEARTS_VERSION_PATCH=${DEARTS_VERSION_PATCH}
    DEARTS_VERSION="${DEARTS_VERSION}"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEARTS_DEBUG)
endif()

if(DEARTS_ENABLE_LOGGING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEARTS_ENABLE_LOGGING)
endif()

if(DEARTS_ENABLE_PROFILING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEARTS_ENABLE_PROFILING)
endif()


# 资源文件处理
if(EXISTS ${CMAKE_SOURCE_DIR}/resources)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    )
endif()

# Windows平台：复制SDL2.dll到输出目录
if(WIN32)
    # 复制SDL2动态库
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_LIB_DIR}/SDL2.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL2.dll
    )
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_TTF_LIB_DIR}/SDL2_ttf.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL2_ttf.dll
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_IMAGE_LIB_DIR}/SDL2_image.dll
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL2_image.dll
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_MIXER_LIB_DIR}/SDL2_mixer.dll
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL2_mixer.dll
    )
    
    # 创建插件目录
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/builtin
    )
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/external
    )
endif()

# 安装配置
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

if(WIN32)
    install(FILES
        ${SDL2_LIB_DIR}/SDL2.dll
        ${SDL2_TTF_LIB_DIR}/SDL2_ttf.dll
        DESTINATION bin
    )
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/resources)
    install(DIRECTORY resources/
        DESTINATION share/dearts/resources
    )
endif()

# 测试配置
if(DEARTS_BUILD_TESTS AND EXISTS ${CMAKE_SOURCE_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

# 文档配置
if(DEARTS_BUILD_DOCS AND EXISTS ${CMAKE_SOURCE_DIR}/docs)
    add_subdirectory(docs)
endif()

# 示例配置
if(DEARTS_BUILD_EXAMPLES AND EXISTS ${CMAKE_SOURCE_DIR}/examples)
    add_subdirectory(examples)
endif()

# 配置文件生成
configure_file(
    ${CMAKE_SOURCE_DIR}/core/dearts_config.h.in
    ${CMAKE_BINARY_DIR}/include/dearts/dearts_config.h
    @ONLY
)

# 构建信息输出
message(STATUS "")
message(STATUS "=== DearTs Build Configuration ===")
message(STATUS "Version: ${DEARTS_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Build Tests: ${DEARTS_BUILD_TESTS}")
message(STATUS "Build Docs: ${DEARTS_BUILD_DOCS}")
message(STATUS "Build Examples: ${DEARTS_BUILD_EXAMPLES}")
message(STATUS "Enable Logging: ${DEARTS_ENABLE_LOGGING}")
message(STATUS "Enable Profiling: ${DEARTS_ENABLE_PROFILING}")
message(STATUS "=================================")

