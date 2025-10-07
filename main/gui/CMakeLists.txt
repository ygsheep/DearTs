# DearTs GUI Library CMakeLists.txt
# GUI库构建配置

cmake_minimum_required(VERSION 3.16)

# 设置包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/lib/libdearts/include
    ${SDL2_ROOT}/include
    ${SDL2_TTF_ROOT}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# 收集GUI源文件
file(GLOB_RECURSE GUI_SOURCES
    "source/*.cpp"
)

# 收集GUI头文件
file(GLOB_RECURSE GUI_HEADERS
    "include/*.h"
    "include/*.hpp"
)

# 创建GUI静态库
add_library(DearTsGUI STATIC
    ${GUI_SOURCES}
    ${GUI_HEADERS}
)

# 链接依赖库
target_link_libraries(DearTsGUI
    PUBLIC
        DearTsCore
        libdearts
        imgui
    PRIVATE
        ${SDL2_LIB_DIR}/SDL2.lib
        ${SDL2_LIB_DIR}/SDL2main.lib
        ${SDL2_TTF_LIB_DIR}/SDL2_ttf.lib
        ${SDL2_IMAGE_LIB_DIR}/SDL2_image.lib
        ${SDL2_MIXER_LIB_DIR}/SDL2_mixer.lib
)

# Windows特定库
if(WIN32)
    target_link_libraries(DearTsGUI
        PRIVATE
            user32
            gdi32
            shell32
            ole32
            oleaut32
            imm32
            winmm
            version
    )
    
    target_compile_definitions(DearTsGUI
        PUBLIC
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
    )
endif()

# 设置C++标准
target_compile_features(DearTsGUI PUBLIC cxx_std_20)

# 设置输出目录
set_target_properties(DearTsGUI PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)