# Clipboard Parser Plugin CMakeLists.txt

# 设置插件源文件
set(CLIPBOARD_PARSER_PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/clipboard_parser_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/views/clipboard_parser_view.cpp
)

# 设置插件头文件
set(CLIPBOARD_PARSER_PLUGIN_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/clipboard_parser_plugin.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/views/clipboard_parser_view.hpp
)

# 添加 cppjieba 包含路径
include_directories(${CMAKE_SOURCE_DIR}/third_party/cppjieba/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/cppjieba/deps/limonp/include)

# 创建插件库
add_library(clipboard_parser_plugin STATIC
    ${CLIPBOARD_PARSER_PLUGIN_SOURCES}
    ${CLIPBOARD_PARSER_PLUGIN_HEADERS}
)

# 链接必要的库
target_link_libraries(clipboard_parser_plugin
    PRIVATE
    imgui_full
    deartsdl_logger
)

# Windows 平台特定设置
if(WIN32)
    target_link_libraries(clipboard_parser_plugin PRIVATE user32)
    target_compile_definitions(clipboard_parser_plugin PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# 设置包含目录
target_include_directories(clipboard_parser_plugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 设置 C++20 标准
target_compile_features(clipboard_parser_plugin PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(clipboard_parser_plugin PRIVATE /utf-8)
endif()
