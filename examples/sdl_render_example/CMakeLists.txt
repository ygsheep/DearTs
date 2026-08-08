cmake_minimum_required(VERSION 3.20)

# SDL3 + ImGui 混合渲染示例
project(sdl_render_example)

# 可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${DEARTS_ROOT}
    ${DEARTS_ROOT}/third_party/SDL/include
    ${DEARTS_ROOT}/third_party/imgui
    ${DEARTS_ROOT}/third_party/imgui/backends
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
        imgui
        imgui_sdl3
        SDL3-static
        freetype
        lunasvg::lunasvg
)

# Windows 特定设置
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
    )
endif()

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 拷贝 ImGui 配置文件
configure_file(
    ${DEARTS_ROOT}/third_party/imgui/imconfig.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui_user_config.h
    COPYONLY
)

# ================ 复制资源文件 ================

# 定义资源目录
set(RESOURCE_SOURCE_DIR ${DEARTS_ROOT}/resources)
set(RESOURCE_DEST_DIR ${CMAKE_BINARY_DIR}/bin/resources)

# 复制字体文件到输出目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}/fonts
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${RESOURCE_SOURCE_DIR}/fonts
        ${RESOURCE_DEST_DIR}/fonts
    COMMENT "Copying font files to output directory"
)

message(STATUS "SDL3 + ImGui 混合渲染示例配置完成")
