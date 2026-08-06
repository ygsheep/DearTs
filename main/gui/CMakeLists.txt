# DearTs 应用程序 CMakeLists.txt
# 基于 DearTs 框架的 GUI 应用（链接 dearts 静态库）

cmake_minimum_required(VERSION 3.20)
project(DearTs)

# DearTs 可执行文件
add_executable(${PROJECT_NAME}
    include/dearts_application.hpp
    source/main.cpp
    source/dearts_application.cpp
    # Windows 资源文件（仅 Windows）
    ${CMAKE_SOURCE_DIR}/resources/dearts.rc
)

# 包含目录（框架头文件由 dearts PUBLIC 传递）
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/plugins/wuthering_waves/include
)

# 链接 dearts 框架库（core + 内置插件 + 第三方依赖）
target_link_libraries(${PROJECT_NAME} PRIVATE
        dearts
        # 独立插件库（静态编译）
        wuthering_waves_plugin
)

# 平台特定的链接库
if(UNIX AND NOT APPLE)
    # Linux 需要 dl 库用于动态加载
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
endif()

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
endif()

# 定义资源目录
set(RESOURCE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
set(RESOURCE_DEST_DIR ${CMAKE_BINARY_DIR}/bin/resources)

# 复制所有资源文件到输出目录（仅在文件真正变化时复制）
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # 使用 copy_if_different 避免不必要的时间戳更新
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}/fonts
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}/images
    # 复制字体文件（仅在变化时）
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${RESOURCE_SOURCE_DIR}/fonts
        ${RESOURCE_DEST_DIR}/fonts
    # 复制图片文件（仅在变化时）
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${RESOURCE_SOURCE_DIR}/images
        ${RESOURCE_DEST_DIR}/images
    # 复制图标文件（仅在变化时）
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#        ${RESOURCE_SOURCE_DIR}/icon.ico
#        ${CMAKE_BINARY_DIR}/bin/icon.ico
    # SDL3_image.dll 由 SDL_image 项目自动构建到 bin/ 目录，无需手动复制
    # 复制 FFmpeg DLLs（如果 FFmpeg 支持启用）
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<IF:$<BOOL:DEARTS_FFMPEG_SUPPORT>,D:/Programs/vcpkg/installed/x64-windows/debug/bin/avcodec-62.dll,nul>
        ${CMAKE_BINARY_DIR}/bin/avcodec-62.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<IF:$<BOOL:DEARTS_FFMPEG_SUPPORT>,D:/Programs/vcpkg/installed/x64-windows/debug/bin/avformat-62.dll,nul>
        ${CMAKE_BINARY_DIR}/bin/avformat-62.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<IF:$<BOOL:DEARTS_FFMPEG_SUPPORT>,D:/Programs/vcpkg/installed/x64-windows/debug/bin/avutil-60.dll,nul>
        ${CMAKE_BINARY_DIR}/bin/avutil-60.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<IF:$<BOOL:DEARTS_FFMPEG_SUPPORT>,D:/Programs/vcpkg/installed/x64-windows/debug/bin/swresample-6.dll,nul>
        ${CMAKE_BINARY_DIR}/bin/swresample-6.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<IF:$<BOOL:DEARTS_FFMPEG_SUPPORT>,D:/Programs/vcpkg/installed/x64-windows/debug/bin/swscale-9.dll,nul>
        ${CMAKE_BINARY_DIR}/bin/swscale-9.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<IF:$<BOOL:DEARTS_FFMPEG_SUPPORT>,D:/Programs/vcpkg/installed/x64-windows/debug/bin/avfilter-11.dll,nul>
        ${CMAKE_BINARY_DIR}/bin/avfilter-11.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<IF:$<BOOL:DEARTS_FFMPEG_SUPPORT>,D:/Programs/vcpkg/installed/x64-windows/debug/bin/avdevice-62.dll,nul>
        ${CMAKE_BINARY_DIR}/bin/avdevice-62.dll
    COMMENT "Copying resources to output directory (if changed)"
)

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
message(STATUS "  Framework: dearts (static library)")
message(STATUS "  ImGui: Integrated")
message(STATUS "  Theme System: Enabled")
message(STATUS "  Shortcut System: Enabled")
message(STATUS "  Command Palette: Enabled")
message(STATUS "  Custom Title Bar: Enabled")
message(STATUS "  Docking: Enabled")
message(STATUS "  Config Management: Enabled")
