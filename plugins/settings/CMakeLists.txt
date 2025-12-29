# Settings Plugin CMakeLists.txt

# 设置插件源文件
set(SETTINGS_PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/settings_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/settings_view.cpp
)

# 设置插件头文件
set(SETTINGS_PLUGIN_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/settings_plugin.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/views/settings_view.hpp
)
