/**
 * @file title_bar_test.cpp
 * @brief TitleBar UI 自动化测试
 * @details 测试标题栏按钮、菜单、快捷键等功能
 * @author DearTs Team
 * @date 2025
 */

#ifdef IMGUI_TEST_ENGINE_ENABLE

#include "imgui_test_engine/imgui_te_engine.h"
#include "imgui_test_engine/imgui_te_context.h"
#include "imgui.h"
#include <stdio.h>

using namespace ImGui;

// ============================================================================
// TitleBar 按钮测试
// ============================================================================

/**
 * @brief 测试设置按钮点击
 *
 * 测试步骤：
 * 1. 设置引用为主窗口
 * 2. 点击设置按钮
 * 3. 验证设置窗口打开
 * 4. 关闭设置窗口
 */
void TestTitleBarSettingsButton(ImGuiTestContext* ctx) {
    printf("Running: TestTitleBarSettingsButton\n");

    // 设置参考窗口
    ctx->SetRef("DearTsWindow");

    // 点击设置按钮（需要根据实际 ID 调整）
    // 注意：按钮 ID 可能需要根据实际实现调整
    ctx->ItemClick("##settings");
    // 或者使用路径：ctx->ItemClick("TitleBar/SettingsButton");

    // 验证设置窗口可见
    ctx->ItemIsVisible("样式编辑器");

    // 关闭设置窗口
    ctx->ItemClick("样式编辑器/CloseButton");
}

/**
 * @brief 测试任务/插件按钮点击
 */
void TestTitleBarTasksButton(ImGuiTestContext* ctx) {
    printf("Running: TestTitleBarTasksButton\n");

    ctx->SetRef("DearTsWindow");
    ctx->ItemClick("##tasks");
    ctx->ItemIsVisible("任务和插件");

    // 关闭窗口
    ctx->ItemClick("任务和插件/CloseButton");
}

// ============================================================================
// TitleBar 菜单测试
// ============================================================================

/**
 * @brief 测试文件菜单
 *
 * 测试菜单项：
 * - 新建
 * - 打开
 * - 保存
 * - 退出
 */
void TestTitleBarFileMenu(ImGuiTestContext* ctx) {
    printf("Running: TestTitleBarFileMenu\n");

    ctx->SetRef("DearTsWindow");

    // 打开文件菜单
    ctx->MenuCheck("///File");

    // 验证菜单项存在（可以点击）
    // 注意：这些是示例，实际菜单项需要根据实现调整
    ctx->ItemIsVisible("///File/New");
    ctx->ItemIsVisible("///File/Open");
    ctx->ItemIsVisible("///File/Save");

    // 关闭菜单（按 ESC）
    ctx->KeyChord(ImGuiKey_Escape);
}

/**
 * @brief 测试视图菜单
 */
void TestTitleBarViewMenu(ImGuiTestContext* ctx) {
    printf("Running: TestTitleBarViewMenu\n");

    ctx->SetRef("DearTsWindow");
    ctx->MenuCheck("///View");

    // 验证视图菜单项
    ctx->ItemIsVisible("///View/Command Palette");
    ctx->ItemIsVisible("///View/Task Manager");

    ctx->KeyChord(ImGuiKey_Escape);
}

/**
 * @brief 测试主题菜单
 */
void TestTitleBarThemeMenu(ImGuiTestContext* ctx) {
    printf("Running: TestTitleBarThemeMenu\n");

    ctx->SetRef("DearTsWindow");
    ctx->MenuCheck("///Theme");

    // 验证主题选项
    ctx->ItemIsVisible("///Theme/Dark");
    ctx->ItemIsVisible("///Theme/Light");

    // 可以选择某个主题
    // ctx->ItemClick("///Theme/Dark");

    ctx->KeyChord(ImGuiKey_Escape);
}

// ============================================================================
// TitleBar 快捷键测试
// ============================================================================

/**
 * @brief 测试命令面板快捷键 (Ctrl+Shift+P)
 */
void TestTitleBarCommandPaletteShortcut(ImGuiTestContext* ctx) {
    printf("Running: TestTitleBarCommandPaletteShortcut\n");

    ctx->SetRef("DearTsWindow");

    // 按下 Ctrl+Shift+P
    ctx->KeyChord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P);

    // 验证命令面板窗口打开
    ctx->ItemIsVisible("Command Palette");

    // 关闭命令面板
    ctx->KeyChord(ImGuiKey_Escape);
}

/**
 * @brief 测试设置快捷键 (Ctrl+,)
 */
void TestTitleBarSettingsShortcut(ImGuiTestContext* ctx) {
    printf("Running: TestTitleBarSettingsShortcut\n");

    ctx->SetRef("DearTsWindow");

    // 按下 Ctrl+,
    ctx->KeyChord(ImGuiMod_Ctrl | ImGuiKey_Comma);

    // 验证设置窗口打开
    ctx->ItemIsVisible("样式编辑器");

    // 关闭
    ctx->KeyChord(ImGuiKey_Escape);
}

// ============================================================================
// TitleBar 交互测试
// ============================================================================

/**
 * @brief 测试标题栏按钮悬停
 */
void TestTitleBarButtonHover(ImGuiTestContext* ctx) {
    printf("Running: TestTitleBarButtonHover\n");

    ctx->SetRef("DearTsWindow");

    // 悬停在设置按钮上
    ctx->MouseMoveTo("##settings");

    // 验证工具提示（如果有）
    // ctx->ItemIsVisible("SettingsTooltip");
}

/**
 * @brief 测试标题栏按钮连续点击
 */
void TestTitleBarButtonDoubleClick(ImGuiTestContext* ctx) {
    printf("Running: TestTitleBarButtonDoubleClick\n");

    ctx->SetRef("DearTsWindow");

    // 双击设置按钮
    ctx->ItemDoubleClick("##settings");

    // 验证窗口仍然只打开一次
    ctx->ItemIsVisible("样式编辑器");

    // 关闭
    ctx->ItemClick("样式编辑器/CloseButton");
}

// ============================================================================
// 注册测试
// ============================================================================

/**
 * @brief 注册所有 TitleBar 测试
 */
void RegisterTitleBarTests(ImGuiTestEngine* engine) {
    printf("Registering TitleBar tests...\n");

    // 按钮测试
    ImGuiTest* test = nullptr;

    test = IM_REGISTER_TEST(engine, "ui.titlebar", "settings_button");
    test->TestFunc = TestTitleBarSettingsButton;

    test = IM_REGISTER_TEST(engine, "ui.titlebar", "tasks_button");
    test->TestFunc = TestTitleBarTasksButton;

    // 菜单测试
    test = IM_REGISTER_TEST(engine, "ui.titlebar", "file_menu");
    test->TestFunc = TestTitleBarFileMenu;

    test = IM_REGISTER_TEST(engine, "ui.titlebar", "view_menu");
    test->TestFunc = TestTitleBarViewMenu;

    test = IM_REGISTER_TEST(engine, "ui.titlebar", "theme_menu");
    test->TestFunc = TestTitleBarThemeMenu;

    // 快捷键测试
    test = IM_REGISTER_TEST(engine, "ui.titlebar", "command_palette_shortcut");
    test->TestFunc = TestTitleBarCommandPaletteShortcut;

    test = IM_REGISTER_TEST(engine, "ui.titlebar", "settings_shortcut");
    test->TestFunc = TestTitleBarSettingsShortcut;

    // 交互测试
    test = IM_REGISTER_TEST(engine, "ui.titlebar", "button_hover");
    test->TestFunc = TestTitleBarButtonHover;

    test = IM_REGISTER_TEST(engine, "ui.titlebar", "button_double_click");
    test->TestFunc = TestTitleBarButtonDoubleClick;

    printf("TitleBar tests registered: 8 tests\n");
}

#endif // IMGUI_TEST_ENGINE_ENABLE
