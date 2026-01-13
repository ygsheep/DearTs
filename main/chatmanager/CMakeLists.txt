# ChatManager Application CMakeLists.txt

cmake_minimum_required(VERSION 3.20)

# 定义项目
project(ChatManager VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 收集源文件
set(CHATMANAGER_SOURCES
    source/main.cpp
)

set(CHATMANAGER_HEADERS
    include/chatmanager/application.hpp
)

# 创建可执行文件
add_executable(ChatManager WIN32 ${CHATMANAGER_SOURCES} ${CHATMANAGER_HEADERS})

# 设置包含目录
target_include_directories(ChatManager PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/main/chatmanager/include
    ${CMAKE_SOURCE_DIR}/plugins/chat/include
    ${CMAKE_SOURCE_DIR}/core
)

# 链接库
target_link_libraries(ChatManager PRIVATE
    dearts_core
    chat
)

# 链接系统库
if(UNIX AND NOT APPLE)
    target_link_libraries(ChatManager PRIVATE
        pthread
        dl
    )
endif()

# Windows 特定设置
if(WIN32)
    # 链接 Windows 库
    target_link_libraries(ChatManager PRIVATE
        winhttp
    )
endif()

# 编译选项
if(MSVC)
    target_compile_options(ChatManager PRIVATE
        /W4
        /std:c++20
        /MP
    )
else()
    target_compile_options(ChatManager PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# 设置输出目录
set_target_properties(ChatManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 安装规则
install(TARGETS ChatManager
    RUNTIME DESTINATION bin
)

# 复制必要的资源文件（如果有）
# install(DIRECTORY resources/ DESTINATION bin/resources)
