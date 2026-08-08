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
        ${DEARTS_ROOT}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(toast_notification_plugin
    PUBLIC
        dearts
        deartsdl_logger
)

# 设置 C++20 标准
target_compile_features(toast_notification_plugin PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(toast_notification_plugin PRIVATE /utf-8)
endif()
