cmake_minimum_required(VERSION 3.20)

# ================ WhaleDock 灵动岛 Theme 验证示例 ================
# 仅依赖 SDL3 + ImGui（无 workx / dearts 依赖），用于验证 theme.hpp 视觉效果。
# 渲染层：SDL3_Renderer（跨平台一份代码，自动选择 D3D11/Metal/Vulkan 后端）。
# ==================================================================

add_executable(whale_dock_example
    main.cpp
)

target_include_directories(whale_dock_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DEARTS_ROOT}/third_party/SDL/include
    ${DEARTS_ROOT}/third_party/imgui
    ${DEARTS_ROOT}/third_party/imgui/backends
)

target_link_libraries(whale_dock_example PRIVATE
    imgui
    imgui_sdl3
    SDL3-shared
)

if(MSVC)
    target_compile_features(whale_dock_example PRIVATE cxx_std_20)
    target_compile_options(whale_dock_example PRIVATE /utf-8)
    target_compile_definitions(whale_dock_example PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
    )
endif()

# 拷贝字体资源到可执行文件输出目录（OPPOSans-M.ttf 用于中文渲染）
# 可执行文件在 ${CMAKE_BINARY_DIR}/bin/<config>/，字体需在同级的 resources/fonts/
set(RESOURCE_SOURCE_DIR ${DEARTS_ROOT}/resources)
set(RESOURCE_DEST_DIR ${CMAKE_BINARY_DIR}/bin/resources)
add_custom_command(TARGET whale_dock_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}/fonts
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${RESOURCE_SOURCE_DIR}/fonts
        ${RESOURCE_DEST_DIR}/fonts
    COMMENT "Copying font files to output directory"
)

# Windows: 确保 SDL3.dll 在输出目录
if(WIN32)
    add_custom_command(TARGET whale_dock_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3-shared>
            $<TARGET_FILE_DIR:whale_dock_example>
        COMMENT "Copying SDL3.dll to output directory"
    )
endif()

message(STATUS "WhaleDock theme example configured: whale_dock_example")
