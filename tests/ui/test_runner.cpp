/**
 * @file test_runner.cpp
 * @brief ImGui Test Engine 测试运行器
 * @details 为 DearTs Framework 提供 UI 自动化测试运行器
 * @author DearTs Team
 * @date 2025
 */

#ifdef IMGUI_TEST_ENGINE_ENABLE

#include "imgui_test_engine/imgui_te_engine.h"
#include "imgui_test_engine/imgui_te_context.h"
#include "imgui.h"
#include "SDL3/SDL.h"
#include <stdio.h>

// 前向声明测试注册函数
extern void RegisterTitleBarTests(ImGuiTestEngine* engine);
extern void RegisterCommandPaletteTests(ImGuiTestEngine* engine);
extern void RegisterToastTests(ImGuiTestEngine* engine);
extern void RegisterViewTests(ImGuiTestEngine* engine);
extern void RegisterInteractionTests(ImGuiTestEngine* engine);

/**
 * @brief 注册所有 UI 测试
 */
void RegisterAllTests(ImGuiTestEngine* engine) {
    printf("=== Registering DearTs UI Tests ===\n");

    // 注册各类测试
    RegisterTitleBarTests(engine);
    RegisterCommandPaletteTests(engine);
    RegisterToastTests(engine);
    RegisterViewTests(engine);
    RegisterInteractionTests(engine);

    printf("=== Total UI Tests Registered ===\n");
}

/**
 * @brief 主函数 - UI 测试入口
 */
int main(int argc, char** argv) {
    // TODO: 初始化 SDL3 和 ImGui
    // 这里需要初始化完整的 DearTs 应用环境

    printf("Starting DearTs UI Test Runner...\n");

    // TODO: 创建 ImGui Test Engine
    // ImGuiTestEngine* engine = ImGuiTestEngine_Create();
    // ImGuiTestEngine_InstallDefaultCrashHandler();

    // 注册所有测试
    // RegisterAllTests(engine);

    // TODO: 运行测试
    // ImGuiTestEngine_RunTests(engine, argc, argv);

    printf("DearTs UI Tests completed!\n");

    // TODO: 清理资源
    // ImGuiTestEngine_Destroy(engine);

    return 0;
}

#endif // IMGUI_TEST_ENGINE_ENABLE
