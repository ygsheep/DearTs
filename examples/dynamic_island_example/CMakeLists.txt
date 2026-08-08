cmake_minimum_required(VERSION 3.21)

# ================ workx (Agent Harness) 嵌入 ================
# 灵动岛需要订阅 workx EventBus 事件，以子工程方式嵌入 workx。
# 这是外部工程消费模式（等价于 find_package(workx)）：workx 保持纯库，
# 只关掉其自身 tests/examples/tree-sitter，其余不动。
set(WORKX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WORKX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WORKX_BUILD_CONSUMER OFF CACHE BOOL "" FORCE)
set(WORKX_WITH_TREE_SITTER OFF CACHE BOOL "" FORCE)
set(WORKX_FETCH_GRAMMARS OFF CACHE BOOL "" FORCE)

# workx 依赖 vcpkg 的 curl / nlohmann_json（CONFIG 查找）：
# 优先 workx 自身 manifest 安装的实例，其次 vcpkg classic 全局实例
set(WORKX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../workx"
    CACHE PATH "workx (Agent Harness) 仓库路径" FORCE)

# DearTs 根 include_directories(${DEARTS_ROOT}) 等会传播进 workx 子工程，
# 使 "core/config/config_manager.h" 命中 DearTs 版（namespace DearTs::Core）而非
# workx 版（namespace agent）；"liblogger/logger.h" 命中 DearTs 版；<nlohmann/json.hpp>
# 命中 DearTs 的 third_party/json submodule（版本与 workx vcpkg 依赖不一致）。
# 注意：workx 的 vcpkg_installed 里也有 imgui（1.91.9），必须让 DearTs 的
# imgui（1.92.6，与 backends 匹配）排在它之前，否则 <imgui.h> 版本错位。
# 此处把 DearTs imgui + workx 自身依赖全部前置，使其与各自自构建环境一致。
# 注意：include_directories(BEFORE) 按参数逐个前插（顺序会反转），
# 故此处按期望顺序倒序书写，最终生效顺序为：
#   DearTs imgui → workx/lib → workx vcpkg include → workx/src
include_directories(BEFORE
    "${WORKX_DIR}/src"
    "${WORKX_DIR}/build/vcpkg_installed/x64-windows/include"
    "${WORKX_DIR}/lib"
    "${DEARTS_ROOT}/third_party/imgui/backends"
    "${DEARTS_ROOT}/third_party/imgui"
)
if(NOT CURL_FOUND)
    foreach(_cand
            "${WORKX_DIR}/build/vcpkg_installed/x64-windows"
            "$ENV{VCPKG_ROOT}/installed/x64-windows")
        if(EXISTS "${_cand}/share/curl/CURLConfig.cmake")
            list(APPEND CMAKE_PREFIX_PATH "${_cand}")
            message(STATUS "workx deps: CMAKE_PREFIX_PATH += ${_cand}")
        endif()
    endforeach()
endif()
add_subdirectory(${WORKX_DIR} workx_external)

# ================ 灵动岛示例 ================
add_executable(dynamic_island_example
    main.cpp
    dynamic_island.cpp
    event_bridge.cpp
)

target_include_directories(dynamic_island_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DEARTS_ROOT}/third_party/SDL/include
    ${DEARTS_ROOT}/third_party/imgui
    ${DEARTS_ROOT}/third_party/imgui/backends
)

target_link_libraries(dynamic_island_example PRIVATE
    imgui
    imgui_sdl3
    SDL3-static
    workx_core
)

if(MSVC)
    target_compile_features(dynamic_island_example PRIVATE cxx_std_20)
    target_compile_options(dynamic_island_example PRIVATE /utf-8)
    target_compile_definitions(dynamic_island_example PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
    )
endif()

# 拷贝字体资源到输出目录
set(RESOURCE_SOURCE_DIR ${DEARTS_ROOT}/resources)
set(RESOURCE_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/resources)
add_custom_command(TARGET dynamic_island_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${RESOURCE_SOURCE_DIR}/fonts
        ${RESOURCE_DEST_DIR}/fonts
    COMMENT "Copying font files to output directory"
)

message(STATUS "Dynamic island example configured: dynamic_island_example")
