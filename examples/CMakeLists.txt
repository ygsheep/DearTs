# DearTs Examples CMakeLists.txt
# 示例程序构建配置

cmake_minimum_required(VERSION 3.16)

# 使用与主项目相同的SDL2配置
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/lib/third_party)
set(IMGUI_DIR ${THIRD_PARTY_DIR}/imgui)
set(SDL2_ROOT ${THIRD_PARTY_DIR}/SDL2)
set(SDL2_TTF_ROOT ${THIRD_PARTY_DIR}/SDL2_ttf)
set(SDL2_IMAGE_ROOT ${THIRD_PARTY_DIR}/SDL2_image)
set(SDL2_MIXER_ROOT ${THIRD_PARTY_DIR}/SDL2_mixer)

# 根据平台选择库路径
if(CMAKE_SIZEOF_VOID_P EQUAL 8)  # 64位系统
    set(SDL2_LIB_DIR ${SDL2_ROOT}/lib/x64)
    set(SDL2_TTF_LIB_DIR ${SDL2_TTF_ROOT}/lib/x64)
    set(SDL2_IMAGE_LIB_DIR ${SDL2_IMAGE_ROOT}/lib/x64)
    set(SDL2_MIXER_LIB_DIR ${SDL2_MIXER_ROOT}/lib/x64)
else()  # 32位系统
    set(SDL2_LIB_DIR ${SDL2_ROOT}/lib/x86)
    set(SDL2_TTF_LIB_DIR ${SDL2_TTF_ROOT}/lib/x86)
    set(SDL2_IMAGE_LIB_DIR ${SDL2_IMAGE_ROOT}/lib/x86)
    set(SDL2_MIXER_LIB_DIR ${SDL2_MIXER_ROOT}/lib/x86)
endif()

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${SDL2_ROOT}/include
    ${SDL2_TTF_ROOT}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# 集成测试应用程序
add_executable(integration_test
    examples_main.cpp
)

# 链接库
target_link_libraries(integration_test
    DearTsCore
    imgui
    ${SDL2_LIB_DIR}/SDL2.lib 
    ${SDL2_LIB_DIR}/SDL2main.lib
    ${SDL2_TTF_LIB_DIR}/SDL2_ttf.lib
)

# 编译选项
target_compile_features(integration_test PRIVATE cxx_std_20)

# Windows特定设置
if(WIN32)
    target_link_libraries(integration_test
        user32
        gdi32
        winmm
        imm32
        ole32
        oleaut32
        version
        uuid
        advapi32
        setupapi
        shell32
    )
    
    # 设置为控制台应用程序（便于查看日志）
    set_target_properties(integration_test PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# 设置输出目录
set_target_properties(integration_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

# 复制必要的DLL文件（Windows）
if(WIN32)
    # 复制SDL2 DLL
    if(EXISTS "${SDL2_LIBDIR}/SDL2.dll")
        add_custom_command(TARGET integration_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_LIBDIR}/SDL2.dll"
            $<TARGET_FILE_DIR:integration_test>
        )
    endif()
endif()

# 安装规则
install(TARGETS integration_test
    RUNTIME DESTINATION bin
)

# 添加测试
enable_testing()
add_test(NAME integration_test_run
    COMMAND integration_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)