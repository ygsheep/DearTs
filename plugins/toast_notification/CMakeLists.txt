# Toast Notification Plugin

add_library(toast_notification_plugin OBJECT
    # 头文件
    include/toast.hpp
    include/toast_manager.hpp
    include/toast_view.hpp
    include/toast_plugin.hpp

    # 源文件
    source/toast_manager.cpp
    source/toast_plugin.cpp
)

target_include_directories(toast_notification_plugin
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(toast_notification_plugin
    PUBLIC
        dear_core
        liblogger
)

# 设置 C++20 标准
target_compile_features(toast_notification_plugin PUBLIC cxx_std_20)
