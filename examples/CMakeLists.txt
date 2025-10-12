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

# 字体测试Demo (独立构建，不依赖core)
if(EXISTS ${CMAKE_SOURCE_DIR}/examples/font_test_demo.cpp)
    add_executable(font_test_demo ${CMAKE_SOURCE_DIR}/examples/font_test_demo.cpp)

    target_include_directories(font_test_demo PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${SDL2_ROOT}/include
    )

    target_link_libraries(font_test_demo PRIVATE
        imgui
        ${SDL2_LIB_DIR}/SDL2.lib
        ${SDL2_LIB_DIR}/SDL2main.lib
    )

    target_compile_definitions(font_test_demo PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )

    # 设置目标属性
    set_target_properties(font_test_demo PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "font_test_demo"
        DEBUG_POSTFIX "_d"
    )

    # 复制DLL
    if(WIN32)
        add_custom_command(TARGET font_test_demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_LIB_DIR}/SDL2.dll
            $<TARGET_FILE_DIR:font_test_demo>/SDL2.dll
        )

        add_custom_command(TARGET font_test_demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:font_test_demo>/resources
        )
    endif()

    message(STATUS "Font test demo configured")
endif()

# 集成测试应用程序
add_executable(examples
    examples_main.cpp
)

# LayoutManager测试应用程序
add_executable(layout_manager_test
    layout_manager_test.cpp
)

# 侧边栏布局测试应用程序
add_executable(sidebar_layout_test
    sidebar_layout_test.cpp
)

# 分词助手窗口测试应用程序
add_executable(text_segmentation_test
    text_segmentation_test.cpp
    segmentation_test_layout.cpp
)


# 链接库
target_link_libraries(examples
    DearTsCore
    libdearts
    DearTsGUI
    imgui
    ${SDL2_LIB_DIR}/SDL2.lib 
    ${SDL2_LIB_DIR}/SDL2main.lib
    ${SDL2_TTF_LIB_DIR}/SDL2_ttf.lib
    ${SDL2_IMAGE_LIB_DIR}/SDL2_image.lib
    ${SDL2_MIXER_LIB_DIR}/SDL2_mixer.lib
)

target_link_libraries(layout_manager_test
    DearTsCore
    libdearts
    DearTsGUI
    imgui
    ${SDL2_LIB_DIR}/SDL2.lib 
    ${SDL2_LIB_DIR}/SDL2main.lib
    ${SDL2_TTF_LIB_DIR}/SDL2_ttf.lib
    ${SDL2_IMAGE_LIB_DIR}/SDL2_image.lib
    ${SDL2_MIXER_LIB_DIR}/SDL2_mixer.lib
)

target_link_libraries(sidebar_layout_test
    DearTsCore
    libdearts
    DearTsGUI
    imgui
    ${SDL2_LIB_DIR}/SDL2.lib
    ${SDL2_LIB_DIR}/SDL2main.lib
    ${SDL2_TTF_LIB_DIR}/SDL2_ttf.lib
    ${SDL2_IMAGE_LIB_DIR}/SDL2_image.lib
    ${SDL2_MIXER_LIB_DIR}/SDL2_mixer.lib
)

target_link_libraries(text_segmentation_test
    DearTsCore
    libdearts
    DearTsGUI
    imgui
    ${SDL2_LIB_DIR}/SDL2.lib
    ${SDL2_LIB_DIR}/SDL2main.lib
    ${FREETYPE_ROOT}/lib/freetype.lib
)


# 编译选项
target_compile_features(examples PRIVATE cxx_std_20)
target_compile_features(layout_manager_test PRIVATE cxx_std_20)
target_compile_features(sidebar_layout_test PRIVATE cxx_std_20)
target_compile_features(text_segmentation_test PRIVATE cxx_std_20)

# Windows特定设置
if(WIN32)
    target_link_libraries(examples
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
    
    target_link_libraries(layout_manager_test
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
    
    target_link_libraries(sidebar_layout_test
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

    target_link_libraries(text_segmentation_test
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
    set_target_properties(examples PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    
    set_target_properties(layout_manager_test PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    
    set_target_properties(sidebar_layout_test PROPERTIES
        WIN32_EXECUTABLE FALSE
    )

    set_target_properties(text_segmentation_test PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# 设置输出目录
set_target_properties(examples PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

# 复制必要的DLL文件（Windows）
if(WIN32)
    # 复制SDL2 DLL
    add_custom_command(TARGET examples POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_LIB_DIR}/SDL2.dll"
        $<TARGET_FILE_DIR:examples>
    )
    
    add_custom_command(TARGET layout_manager_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_LIB_DIR}/SDL2.dll"
        $<TARGET_FILE_DIR:layout_manager_test>
    )
    
    add_custom_command(TARGET sidebar_layout_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_LIB_DIR}/SDL2.dll"
        $<TARGET_FILE_DIR:sidebar_layout_test>
    )
    
    # 复制SDL2_ttf DLL
    add_custom_command(TARGET examples POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_TTF_LIB_DIR}/SDL2_ttf.dll"
        $<TARGET_FILE_DIR:examples>
    )
    
    add_custom_command(TARGET layout_manager_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_TTF_LIB_DIR}/SDL2_ttf.dll"
        $<TARGET_FILE_DIR:layout_manager_test>
    )
    
    add_custom_command(TARGET sidebar_layout_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_TTF_LIB_DIR}/SDL2_ttf.dll"
        $<TARGET_FILE_DIR:sidebar_layout_test>
    )
    
    add_custom_command(TARGET text_segmentation_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_TTF_LIB_DIR}/SDL2_ttf.dll"
        $<TARGET_FILE_DIR:text_segmentation_test>
    )
    
    # 复制SDL2_image DLL
    add_custom_command(TARGET examples POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_IMAGE_LIB_DIR}/SDL2_image.dll"
        $<TARGET_FILE_DIR:examples>
    )
    
    add_custom_command(TARGET layout_manager_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_IMAGE_LIB_DIR}/SDL2_image.dll"
        $<TARGET_FILE_DIR:layout_manager_test>
    )
    
    add_custom_command(TARGET sidebar_layout_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_IMAGE_LIB_DIR}/SDL2_image.dll"
        $<TARGET_FILE_DIR:sidebar_layout_test>
    )
    
    add_custom_command(TARGET text_segmentation_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_IMAGE_LIB_DIR}/SDL2_image.dll"
        $<TARGET_FILE_DIR:text_segmentation_test>
    )
    
    # 复制SDL2_mixer DLL
    add_custom_command(TARGET examples POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_MIXER_LIB_DIR}/SDL2_mixer.dll"
        $<TARGET_FILE_DIR:examples>
    )
    
    add_custom_command(TARGET layout_manager_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_MIXER_LIB_DIR}/SDL2_mixer.dll"
        $<TARGET_FILE_DIR:layout_manager_test>
    )
    
    add_custom_command(TARGET sidebar_layout_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_MIXER_LIB_DIR}/SDL2_mixer.dll"
        $<TARGET_FILE_DIR:sidebar_layout_test>
    )
    
    add_custom_command(TARGET text_segmentation_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_MIXER_LIB_DIR}/SDL2_mixer.dll"
        $<TARGET_FILE_DIR:text_segmentation_test>
    )
    
    add_custom_command(TARGET text_segmentation_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_MIXER_LIB_DIR}/SDL2_mixer.dll"
        $<TARGET_FILE_DIR:text_segmentation_test>
    )
    
    # 复制SDL2_ttf DLL及依赖的freetype.dll
    add_custom_command(TARGET examples POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_TTF_LIB_DIR}/SDL2_ttf.dll"
        $<TARGET_FILE_DIR:examples>
    )
    
    add_custom_command(TARGET layout_manager_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_TTF_LIB_DIR}/SDL2_ttf.dll"
        $<TARGET_FILE_DIR:layout_manager_test>
    )
    
    add_custom_command(TARGET sidebar_layout_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_TTF_LIB_DIR}/SDL2_ttf.dll"
        $<TARGET_FILE_DIR:sidebar_layout_test>
    )
    
    # 注意：text_segmentation_test不再需要SDL2_ttf.dll，因为我们移除了对SDL2_ttf.lib的链接
    # add_custom_command(TARGET text_segmentation_test POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #     "${SDL2_TTF_LIB_DIR}/SDL2_ttf.dll"
    #     $<TARGET_FILE_DIR:text_segmentation_test>
    # )
    
    # 注意：如果SDL2_ttf目录中包含freetype.dll，则也需要复制
    # add_custom_command(TARGET text_segmentation_test POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #     "${SDL2_TTF_LIB_DIR}/freetype.dll"
    #     $<TARGET_FILE_DIR:text_segmentation_test>
    # )
endif()

# 复制资源文件
if(EXISTS ${CMAKE_SOURCE_DIR}/resources)
    add_custom_command(TARGET examples POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:examples>/resources
    )
    
    add_custom_command(TARGET layout_manager_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:layout_manager_test>/resources
    )
    
    add_custom_command(TARGET sidebar_layout_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:sidebar_layout_test>/resources
    )
endif()

# 安装规则
install(TARGETS examples
    RUNTIME DESTINATION bin
)

# 添加测试
enable_testing()
add_test(NAME examples_run
    COMMAND examples
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)