# Chat Plugin CMakeLists.txt

cmake_minimum_required(VERSION 3.20)

# 定义项目
project(chat_plugin VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 收集源文件
file(GLOB_RECURSE CHAT_SOURCES
    "source/chat/*.cpp"
)

file(GLOB_RECURSE CHAT_HEADERS
    "include/chat/*.hpp"
    "include/chat/**/*.hpp"
)

# 创建静态库
add_library(chat STATIC ${CHAT_SOURCES} ${CHAT_HEADERS})

# 设置包含目录
target_include_directories(chat PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/plugins/chat/include
    ${CMAKE_SOURCE_DIR}/core
)

# 链接 DearTs 核心库
target_link_libraries(chat PUBLIC
    dearts_core
)

# 设置输出目录
set_target_properties(chat PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)

# 编译选项
if(MSVC)
    target_compile_options(chat PRIVATE
        /W4
        /std:c++20
    )
else()
    target_compile_options(chat PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# 安装规则（可选）
install(TARGETS chat
    ARCHIVE DESTINATION plugins
    LIBRARY DESTINATION plugins
)

# 安装头文件
install(FILES ${CHAT_HEADERS}
    DESTINATION include/chat
)
