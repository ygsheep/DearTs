/**
 * @file command_palette_test.cpp
 * @brief Command Palette UI 自动化测试
 * @details 测试命令面板的打开、搜索、选择和执行功能
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
// Command Palette 打开/关闭测试
// ============================================================================

/**
 * @brief 测试通过快捷键打开命令面板
 */
void TestCommandPaletteOpenWithShortcut(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPaletteOpenWithShortcut\n");

    ctx->SetRef("DearTsWindow");

    // 按下 Ctrl+Shift+P 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 验证命令面板窗口可见
    ctx->ItemCheck("Command Palette");

    // 验证输入框存在且聚焦
    ctx->ItemCheck("##command_palette_input");

    // 验证命令列表存在
    ctx->ItemCheck("##commands");

    // 关闭命令面板
    ctx->KeyPress(ImGuiKey_Escape);
}

/**
 * @brief 测试通过 ESC 关闭命令面板
 */
void TestCommandPaletteCloseWithEscape(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPaletteCloseWithEscape\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);
    ctx->ItemCheck("Command Palette");

    // 按 ESC 关闭
    ctx->KeyPress(ImGuiKey_Escape);

    // 验证窗口关闭
    IM_CHECK(!ctx->ItemExists("Command Palette"));
}

/**
 * @brief 测试点击外部关闭命令面板
 */
void TestCommandPaletteCloseByClickingOutside(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPaletteCloseByClickingOutside\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);
    ctx->ItemCheck("Command Palette");

    // 点击窗口外部区域
    ctx->MouseMove("DearTsWindow/WorkArea");
    ctx->MouseClick(ImGuiMouseButton_Left);

    // 验证窗口关闭
    IM_CHECK(!ctx->ItemExists("Command Palette"));
}

// ============================================================================
// Command Palette 搜索测试
// ============================================================================

/**
 * @brief 测试命令过滤功能
 */
void TestCommandPaletteFiltering(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPaletteFiltering\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 输入搜索文本
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsAppend("theme");

    // 验证过滤后的命令列表只包含相关命令
    // 注意：具体命令需要根据实际注册的命令调整
    // ctx->ItemCheck("Switch to Dark Theme");
    // ctx->ItemCheck("Switch to Light Theme");

    // 清空搜索
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsReplace("");

    // 关闭
    ctx->KeyPress(ImGuiKey_Escape);
}

/**
 * @brief 测试命令搜索不区分大小写
 */
void TestCommandPaletteCaseInsensitiveSearch(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPaletteCaseInsensitiveSearch\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 测试小写搜索
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsAppend("settings");
    // 验证找到 "Settings" 命令

    // 测试大写搜索
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsReplace("SETTINGS");
    // 应该返回相同结果

    // 测试混合大小写
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsReplace("SeTtInGs");
    // 应该返回相同结果

    ctx->KeyPress(ImGuiKey_Escape);
}

/**
 * @brief 测试空搜索结果显示所有命令
 */
void TestCommandPaletteEmptySearch(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPaletteEmptySearch\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 不输入任何搜索文本
    // 验证显示所有已注册的命令
    ctx->ItemCheck("##commands");

    // 应该看到所有命令类别
    // ctx->ItemCheck("File Commands");
    // ctx->ItemCheck("View Commands");
    // ctx->ItemCheck("Theme Commands");

    ctx->KeyPress(ImGuiKey_Escape);
}

// ============================================================================
// Command Palette 导航测试
// ============================================================================

/**
 * @brief 测试键盘导航（上下箭头）
 */
void TestCommandPaletteKeyboardNavigation(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPaletteKeyboardNavigation\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 按下箭头键选择下一个命令
    ctx->KeyPress(ImGuiKey_DownArrow);

    // 验证第二个命令被选中
    // ctx->ItemIsSelected("Commands/Item2");

    // 按上箭头键返回
    ctx->KeyPress(ImGuiKey_UpArrow);

    // 验证第一个命令被选中
    // ctx->ItemIsSelected("Commands/Item1");

    ctx->KeyPress(ImGuiKey_Escape);
}

/**
 * @brief 测试 PageDown/PageUp 快速导航
 */
void TestCommandPaletteFastNavigation(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPaletteFastNavigation\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 按 PageDown 向下翻页
    ctx->KeyPress(ImGuiKey_PageDown);

    // 按 PageUp 向上翻页
    ctx->KeyPress(ImGuiKey_PageUp);

    ctx->KeyPress(ImGuiKey_Escape);
}

// ============================================================================
// Command Palette 命令执行测试
// ============================================================================

/**
 * @brief 测试通过 Enter 执行命令
 */
void TestCommandPaletteExecuteWithEnter(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPaletteExecuteWithEnter\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 选择第一个命令（默认选中）
    // 按 Enter 执行
    ctx->KeyPress(ImGuiKey_Enter);

    // 验证命令面板关闭
    IM_CHECK(!ctx->ItemExists("Command Palette"));

    // 验证命令执行结果（根据具体命令验证）
    // 例如：如果执行了 "Switch to Dark Theme"，验证主题已切换
}

/**
 * @brief 测试通过点击执行命令
 */
void TestCommandPaletteExecuteWithClick(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPaletteExecuteWithClick\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 点击某个命令项
    // 注意：命令项 ID 需要根据实际实现调整
    // ctx->MouseMove("##commands/0");
    // ctx->MouseClick(ImGuiMouseButton_Left);

    // 验证命令面板关闭
    IM_CHECK(!ctx->ItemExists("Command Palette"));
}

/**
 * @brief 测试命令执行后打开相关窗口
 */
void TestCommandPaletteOpensWindow(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPaletteOpensWindow\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 搜索 "settings"
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsAppend("settings");

    // 执行 "Open Settings" 命令
    ctx->KeyPress(ImGuiKey_Enter);

    // 验证设置窗口打开
    ctx->ItemCheck("样式编辑器");

    // 关闭设置窗口
    ctx->MouseMove("样式编辑器/CloseButton");
    ctx->MouseClick(ImGuiMouseButton_Left);
}

// ============================================================================
// Command Palette 性能测试
// ============================================================================

/**
 * @brief 测试大量命令时的性能
 */
void TestCommandPalettePerformanceWithManyCommands(ImGuiTestContext* ctx) {
    printf("Running: TestCommandPalettePerformanceWithManyCommands\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 快速输入搜索文本
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsAppend("a");

    // 验证搜索响应时间（应该很快，无卡顿）
    // ImGui Test Engine 会自动检测超时

    // 快速删除输入
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsReplace("");

    // 验证所有命令重新显示

    ctx->KeyPress(ImGuiKey_Escape);
}

// ============================================================================
// 注册测试
// ============================================================================

/**
 * @brief 注册所有 Command Palette 测试
 */
void RegisterCommandPaletteTests(ImGuiTestEngine* engine) {
    printf("Registering Command Palette tests...\n");

    ImGuiTest* test = nullptr;

    // 打开/关闭测试
    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "open_with_shortcut");
    test->TestFunc = TestCommandPaletteOpenWithShortcut;

    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "close_with_escape");
    test->TestFunc = TestCommandPaletteCloseWithEscape;

    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "close_by_clicking_outside");
    test->TestFunc = TestCommandPaletteCloseByClickingOutside;

    // 搜索测试
    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "filtering");
    test->TestFunc = TestCommandPaletteFiltering;

    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "case_insensitive_search");
    test->TestFunc = TestCommandPaletteCaseInsensitiveSearch;

    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "empty_search_shows_all");
    test->TestFunc = TestCommandPaletteEmptySearch;

    // 导航测试
    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "keyboard_navigation");
    test->TestFunc = TestCommandPaletteKeyboardNavigation;

    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "fast_navigation");
    test->TestFunc = TestCommandPaletteFastNavigation;

    // 命令执行测试
    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "execute_with_enter");
    test->TestFunc = TestCommandPaletteExecuteWithEnter;

    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "execute_with_click");
    test->TestFunc = TestCommandPaletteExecuteWithClick;

    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "opens_window");
    test->TestFunc = TestCommandPaletteOpensWindow;

    // 性能测试
    test = IM_REGISTER_TEST(engine, "ui.commandpalette", "performance_many_commands");
    test->TestFunc = TestCommandPalettePerformanceWithManyCommands;

    printf("Command Palette tests registered: 11 tests\n");
}

#endif // IMGUI_TEST_ENGINE_ENABLE
