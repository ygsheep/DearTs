# Settings Plugin CMakeLists.txt

# 设置插件源文件
set(SETTINGS_PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/settings_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/settings_view.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/widgets/toast_settings_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/widgets/theme_settings_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/widgets/character_settings_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/widgets/plugin_manager_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/widgets/dependency_graph_widget.cpp
)

# 设置插件头文件
set(SETTINGS_PLUGIN_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/settings_plugin.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/settings_view.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/widgets/toast_settings_widget.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/widgets/theme_settings_widget.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/widgets/character_settings_widget.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/widgets/plugin_manager_widget.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/widgets/dependency_graph_widget.hpp
)
