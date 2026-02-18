# Memory Core Plugin CMakeLists.txt

cmake_minimum_required(VERSION 3.20)

# 定义项目
project(memory_core_plugin VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 收集源文件
file(GLOB_RECURSE MEMORY_CORE_SOURCES
    "source/memory_core/*.cpp"
)

file(GLOB_RECURSE MEMORY_CORE_HEADERS
    "include/memory_core/*.hpp"
    "include/memory_core/**/*.hpp"
)

# 创建静态库
add_library(memory_core STATIC ${MEMORY_CORE_SOURCES} ${MEMORY_CORE_HEADERS})

# 设置包含目录
target_include_directories(memory_core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/plugins/memory_core/include
    ${CMAKE_SOURCE_DIR}/plugins/chat/include
    ${CMAKE_SOURCE_DIR}/core
)

# 链接 DearTs 核心库（注意：核心使用源码，不需要链接）
# dearts_core 不是实际的库，而是源码集合

# 查找 SQLite3 (可选)
find_package(unofficial-sqlite3 CONFIG QUIET)
if(unofficial-sqlite3_FOUND)
    message(STATUS "SQLite3 found via vcpkg: unofficial-sqlite3")
    target_link_libraries(memory_core PUBLIC unofficial::sqlite3::sqlite3)
    target_compile_definitions(memory_core PRIVATE SQLITE3_FOUND)
else()
    # Linux/Unix: 尝试使用 pkg-config 查找 SQLite3
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SQLite3 sqlite3)
        if(SQLite3_FOUND)
            message(STATUS "SQLite3 found via pkg-config: ${SQLite3_VERSION}")
            target_link_libraries(memory_core PUBLIC ${SQLite3_LIBRARIES})
            target_include_directories(memory_core PUBLIC ${SQLite3_INCLUDE_DIRS})
            target_compile_definitions(memory_core PRIVATE SQLITE3_FOUND)
        else()
            message(WARNING "SQLite3 not found. Memory persistence features will be disabled.")
            target_compile_definitions(memory_core PRIVATE SQLITE3_NOT_FOUND)
        endif()
    else()
        message(WARNING "SQLite3 not found. Memory persistence features will be disabled.")
        target_compile_definitions(memory_core PRIVATE SQLITE3_NOT_FOUND)
    endif()
endif()

# 设置输出目录
set_target_properties(memory_core PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)

# 编译选项
if(MSVC)
    target_compile_options(memory_core PRIVATE
        /W4
        /std:c++20
    )
else()
    target_compile_options(memory_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# 安装规则（可选）
install(TARGETS memory_core
    ARCHIVE DESTINATION plugins
    LIBRARY DESTINATION plugins
)

# 安装头文件
install(FILES ${MEMORY_CORE_HEADERS}
    DESTINATION include/memory_core
)
