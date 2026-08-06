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
    source/application.cpp
)

set(CHATMANAGER_HEADERS
    include/chatmanager/application.hpp
)

# 创建可执行文件（不要使用 WIN32，因为我们使用标准 main 函数）
if(WIN32)
    # Windows: 添加资源文件（用于设置应用程序图标）
    add_executable(ChatManager
        ${CHATMANAGER_SOURCES}
        ${CHATMANAGER_HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/chatmanager.rc
    )
else()
    # 其他平台
    add_executable(ChatManager ${CHATMANAGER_SOURCES} ${CHATMANAGER_HEADERS})
endif()

# 设置包含目录（框架头文件由 dearts PUBLIC 传递）
target_include_directories(ChatManager PRIVATE
    ${CMAKE_SOURCE_DIR}/main/chatmanager/include
    ${CMAKE_SOURCE_DIR}/plugins/chat/include
    ${CMAKE_SOURCE_DIR}/plugins/memory_core/include
)

# 收集 Chat Plugin 源文件
file(GLOB_RECURSE CHAT_PLUGIN_SOURCES
    ${CMAKE_SOURCE_DIR}/plugins/chat/source/*.cpp
)
file(GLOB_RECURSE CHAT_PLUGIN_HEADERS
    ${CMAKE_SOURCE_DIR}/plugins/chat/include/*.hpp
    ${CMAKE_SOURCE_DIR}/plugins/chat/include/*.h
)

# 收集 MemoryCore Plugin 源文件
file(GLOB_RECURSE MEMORY_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/plugins/memory_core/source/*.cpp
)
file(GLOB_RECURSE MEMORY_CORE_HEADERS
    ${CMAKE_SOURCE_DIR}/plugins/memory_core/include/*.hpp
    ${CMAKE_SOURCE_DIR}/plugins/memory_core/include/**/*.hpp
)

# 添加插件源文件（core + builtin 插件由 dearts 库提供）
target_sources(ChatManager PRIVATE
    ${CHAT_PLUGIN_SOURCES}
    ${CHAT_PLUGIN_HEADERS}
    ${MEMORY_CORE_SOURCES}
    ${MEMORY_CORE_HEADERS}
)

# ImGui SDL3 GPU Backend
target_sources(ChatManager PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_sdlgpu3.cpp
)

# 链接 DearTs 框架库（core + 内置插件 + 第三方依赖）
target_link_libraries(ChatManager PRIVATE
    dearts
)

# 查找 SQLite3 (可选)
find_package(unofficial-sqlite3 CONFIG QUIET)
if(unofficial-sqlite3_FOUND)
    message(STATUS "SQLite3 found via vcpkg: unofficial-sqlite3")
    target_link_libraries(ChatManager PRIVATE unofficial::sqlite3::sqlite3)
    target_compile_definitions(ChatManager PRIVATE SQLITE3_FOUND)
else()
    # Linux/Unix: 尝试使用 pkg-config 查找 SQLite3
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SQLite3 sqlite3)
        if(SQLite3_FOUND)
            message(STATUS "SQLite3 found via pkg-config: ${SQLite3_VERSION}")
            target_link_libraries(ChatManager PRIVATE ${SQLite3_LIBRARIES})
            target_include_directories(ChatManager PRIVATE ${SQLite3_INCLUDE_DIRS})
            target_compile_definitions(ChatManager PRIVATE SQLITE3_FOUND)
        else()
            message(WARNING "SQLite3 not found for ChatManager. MemoryCore persistence will be disabled.")
            target_compile_definitions(ChatManager PRIVATE SQLITE3_NOT_FOUND)
        endif()
    else()
        message(WARNING "SQLite3 not found for ChatManager. MemoryCore persistence will be disabled.")
        target_compile_definitions(ChatManager PRIVATE SQLITE3_NOT_FOUND)
    endif()
endif()

# 查找 OpenSSL (用于 HTTPS 支持，如果 vcpkg 安装了 OpenSSL)
if(WIN32)
    # Windows: 使用 vcpkg 安装的 OpenSSL
    find_package(OpenSSL CONFIG QUIET)
    if(OpenSSL_FOUND)
        message(STATUS "OpenSSL found via vcpkg, HTTPS support enabled")
        target_link_libraries(ChatManager PRIVATE OpenSSL::SSL OpenSSL::Crypto)
        target_compile_definitions(ChatManager PRIVATE HAVE_OPENSSL)
        # OpenSSL 库路径
        target_link_directories(ChatManager PRIVATE "D:/Programs/vcpkg/installed/x64-windows/lib")
    else()
        message(WARNING "OpenSSL not found. HTTPS support will be disabled.")
    endif()
endif()

# 链接系统库
if(UNIX AND NOT APPLE)
    target_link_libraries(ChatManager PRIVATE
        pthread
        dl
        boost_asio  # Linux 也需要 Boost.Asio
    )
endif()

# Windows 特定设置
if(WIN32)
    # 链接 Windows 库和 Boost.Asio
    target_link_libraries(ChatManager PRIVATE
        boost_asio      # 使用 Boost.Asio 替代 WinHTTP
        setupapi
    )
    target_compile_definitions(ChatManager PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
    )
    # Release 版本设置为 GUI 应用程序（无控制台窗口）
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(ChatManager PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
        target_link_options(ChatManager PRIVATE
            "/SUBSYSTEM:WINDOWS"
            "/ENTRY:mainCRTStartup"
        )
    endif()
endif()

# 编译选项
if(MSVC)
    target_compile_options(ChatManager PRIVATE
        /W4
        /std:c++20
        /MP
        /Zc:preprocessor
        /utf-8
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

# 资源目录配置
set(RESOURCE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
set(RESOURCE_DEST_DIR ${CMAKE_BINARY_DIR}/bin/resources)

# 安装规则
install(TARGETS ChatManager
    RUNTIME DESTINATION bin
)

# 复制必要的资源文件（如果有）
# install(DIRECTORY resources/ DESTINATION bin/resources)

# 复制所有资源文件到输出目录（仅在文件真正变化时复制）
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        # 使用 copy_if_different 避免不必要的时间戳更新
        COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}/fonts
        # 复制字体文件（仅在变化时）
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${RESOURCE_SOURCE_DIR}/fonts
        ${RESOURCE_DEST_DIR}/fonts

        COMMENT "Copying resources to output directory (if changed)"
)

# 只有在使用预编译 SDL3_image 时才需要手动复制 DLL
if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/SDL_image/lib/x64/SDL3_image.dll)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        # 复制 SDL3_image.dll（根据架构选择）
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/third_party/SDL_image/lib/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/SDL3_image.dll
        ${CMAKE_BINARY_DIR}/bin/SDL3_image.dll

        COMMENT "Copying SDL3_image.dll (precompiled version)"
    )
endif()
