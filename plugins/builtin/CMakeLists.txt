# DearTs Builtin Plugin CMake 配置

cmake_minimum_required(VERSION 3.20)
project(BuiltinPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 插件源文件
set(PLUGIN_SOURCES
    source/builtin_plugin.cpp
)

# 插件头文件
set(PLUGIN_HEADERS
    include/builtin_plugin.hpp
    include/views/hello_world_view.hpp
    include/views/data_inspector_view.hpp
)

# 创建动态库插件
add_library(${PROJECT_NAME} SHARED
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
)

# 包含目录
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/lib/liblogger/include
        ${CMAKE_SOURCE_DIR}/third_party/imgui
        ${CMAKE_SOURCE_DIR}/third_party/SDL/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        # 依赖 DearTs 核心库（如果需要）
        deartsdl_logger
)

# Windows 特定设置
if(WIN32)
    # 导出符号
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        BUILDING_DLL
    )

    # 设置输出目录
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    )
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
)

# 插件元数据
set(PLUGIN_NAME "Builtin")
set(PLUGIN_AUTHOR "DearTs Team")
set(PLUGIN_DESCRIPTION "DearTs 内置插件，提供默认 UI 组件")
set(PLUGIN_VERSION "1.0.0")
set(PLUGIN_API_VERSION "1.0.0")

# 生成插件元数据文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/plugin.json
    @ONLY
)

# 安装插件元数据
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/plugin.json
    DESTINATION plugins
)
