# Command Palette Plugin CMakeLists.txt

# 设置插件源文件
set(COMMAND_PALETTE_PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/command_palette_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/command_palette_view.cpp
)

# 设置插件头文件
set(COMMAND_PALETTE_PLUGIN_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/command_palette_plugin.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/command_palette_view.hpp
)
