/**
 * @file interaction_test.cpp
 * @brief 通用交互测试
 * @details 测试鼠标、键盘、拖放等通用 UI 交互
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
// 鼠标交互测试
// ============================================================================

/**
 * @brief 测试鼠标悬停工具提示
 */
void TestMouseHoverTooltip(ImGuiTestContext* ctx) {
    printf("Running: TestMouseHoverTooltip\n");

    ctx->SetRef("DearTsWindow");

    // 悬停在某个按钮上
    ctx->MouseMove("##settings");

    // 等待工具提示出现
    ctx->Yield(500);

    // 验证工具提示显示
    // ctx->ItemCheck("##settings/Tooltip");
}

/**
 * @brief 测试鼠标右键菜单
 */
void TestMouseContextMenu(ImGuiTestContext* ctx) {
    printf("Running: TestMouseContextMenu\n");

    ctx->SetRef("DearTsWindow");

    // 在工作区右键点击
    ctx->MouseMove("DearTsWindow/WorkArea");
    ctx->MouseClick(ImGuiMouseButton_Right);

    // 验证上下文菜单显示
    // ctx->ItemCheck("##ContextMenu");

    // 按 ESC 关闭菜单
    ctx->KeyPress(ImGuiKey_Escape);
}

// ============================================================================
// 键盘交互测试
// ============================================================================

/**
 * @brief 测试 Tab 键焦点导航
 */
void TestKeyboardTabNavigation(ImGuiTestContext* ctx) {
    printf("Running: TestKeyboardTabNavigation\n");

    ctx->SetRef("DearTsWindow");

    // 打开设置窗口
    ctx->MouseMove("##settings");
    ctx->MouseClick(ImGuiMouseButton_Left);

    // 按 Tab 键在控件间导航
    ctx->KeyPress(ImGuiKey_Tab);

    // 验证焦点移动到下一个控件
    // ctx->ItemIsFocused("Settings/NextControl");

    // 关闭窗口
    ctx->KeyPress(ImGuiKey_Escape);
}

/**
 * @brief 测试快捷键组合
 */
void TestKeyboardShortcuts(ImGuiTestContext* ctx) {
    printf("Running: TestKeyboardShortcuts\n");

    ctx->SetRef("DearTsWindow");

    // 测试 Ctrl+N (新建)
    // ctx->KeyDown(ImGuiMod_Ctrl);
    // ctx->KeyPress(ImGuiKey_N);
    // ctx->KeyUp(ImGuiMod_Ctrl);
    // 验证新建窗口/对话框打开

    // 测试 Ctrl+S (保存)
    // ctx->KeyDown(ImGuiMod_Ctrl);
    // ctx->KeyPress(ImGuiKey_S);
    // ctx->KeyUp(ImGuiMod_Ctrl);
    // 验证保存操作执行

    // 测试 Ctrl+Z (撤销)
    // ctx->KeyDown(ImGuiMod_Ctrl);
    // ctx->KeyPress(ImGuiKey_Z);
    // ctx->KeyUp(ImGuiMod_Ctrl);
    // 验证撤销操作执行
}

// ============================================================================
// 拖放交互测试
// ============================================================================

/**
 * @brief 测试文件拖放到窗口
 */
void TestDragAndDropFile(ImGuiTestContext* ctx) {
    printf("Running: TestDragAndDropFile\n");

    ctx->SetRef("DearTsWindow");

    // 模拟拖放文件到窗口
    // ctx->BeginDrag()
    //     .SetSource("FileExplorer/test.bin")
    //     .SetTarget("DearTsWindow/WorkArea")
    //     .Drop();

    // 验证文件被加载
    // ctx->ItemIsVisible("Hex Editor");
}

/**
 * @brief 测试 View 拖放重新排列
 */
void TestDragAndDropViewReorder(ImGuiTestContext* ctx) {
    printf("Running: TestDragAndDropViewReorder\n");

    ctx->SetRef("DearTsWindow");

    // 打开多个 View
    // 拖动 View 标签重新排列
    // ctx->DragTo("View1/Tab", "View2/Tab");

    // 验证顺序改变
}

// ============================================================================
// 文本输入测试
// ============================================================================

/**
 * @brief 测试文本输入框
 */
void TestTextInput(ImGuiTestContext* ctx) {
    printf("Running: TestTextInput\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 输入文本
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsAppend("test input");

    // 验证文本输入成功
    // ctx->ItemHasValue("##command_palette_input", "test input");

    // 清空输入
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyPress(ImGuiKey_A);  // Ctrl+A 全选
    ctx->KeyUp(ImGuiMod_Ctrl);
    ctx->KeyPress(ImGuiKey_Backspace);            // 删除

    // 验证清空
    // ctx->ItemHasValue("##command_palette_input", "");

    // 关闭
    ctx->KeyPress(ImGuiKey_Escape);
}

// ============================================================================
// 滚动测试
// ============================================================================

/**
 * @brief 测试列表滚动
 */
void TestScrolling(ImGuiTestContext* ctx) {
    printf("Running: TestScrolling\n");

    ctx->SetRef("DearTsWindow");

    // 打开命令面板
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);

    // 使用鼠标滚轮滚动
    // ctx->MouseWheel("##commands", -1.0f);

    // 验证内容滚动
    // 使用键盘滚动
    ctx->KeyPress(ImGuiKey_DownArrow);

    // 验证选择改变

    ctx->KeyPress(ImGuiKey_Escape);
}

// ============================================================================
// 注册测试
// ============================================================================

/**
 * @brief 注册所有交互测试
 */
void RegisterInteractionTests(ImGuiTestEngine* engine) {
    printf("Registering Interaction tests...\n");

    ImGuiTest* test = nullptr;

    // 鼠标交互
    test = IM_REGISTER_TEST(engine, "ui.interactions", "hover_tooltip");
    test->TestFunc = TestMouseHoverTooltip;

    test = IM_REGISTER_TEST(engine, "ui.interactions", "context_menu");
    test->TestFunc = TestMouseContextMenu;

    // 键盘交互
    test = IM_REGISTER_TEST(engine, "ui.interactions", "tab_navigation");
    test->TestFunc = TestKeyboardTabNavigation;

    test = IM_REGISTER_TEST(engine, "ui.interactions", "keyboard_shortcuts");
    test->TestFunc = TestKeyboardShortcuts;

    // 拖放交互
    test = IM_REGISTER_TEST(engine, "ui.interactions", "drag_drop_file");
    test->TestFunc = TestDragAndDropFile;

    test = IM_REGISTER_TEST(engine, "ui.interactions", "drag_drop_view_reorder");
    test->TestFunc = TestDragAndDropViewReorder;

    // 文本输入
    test = IM_REGISTER_TEST(engine, "ui.interactions", "text_input");
    test->TestFunc = TestTextInput;

    // 滚动
    test = IM_REGISTER_TEST(engine, "ui.interactions", "scrolling");
    test->TestFunc = TestScrolling;

    printf("Interaction tests registered: 8 tests\n");
}

#endif // IMGUI_TEST_ENGINE_ENABLE
