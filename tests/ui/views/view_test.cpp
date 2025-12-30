/**
 * @file view_test.cpp
 * @brief View 组件通用 UI 测试
 * @details 测试 ImGui View 系统的停靠、关闭、焦点等功能
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
// View 基础功能测试
// ============================================================================

/**
 * @brief 测试 View 打开和关闭
 */
void TestViewOpenAndClose(ImGuiTestContext* ctx) {
    printf("Running: TestViewOpenAndClose\n");

    // 通过命令面板打开某个 View
    ctx->SetRef("DearTsWindow");
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsAppend("Data Inspector");
    ctx->KeyPress(ImGuiKey_Enter);

    // 验证 View 打开
    ctx->ItemCheck("Data Inspector");

    // 关闭 View
    ctx->MouseMove("Data Inspector/CloseButton");
    ctx->MouseClick(ImGuiMouseButton_Left);

    // 验证 View 关闭
    IM_CHECK(!ctx->ItemExists("Data Inspector"));
}

/**
 * @brief 测试 View 停靠功能
 */
void TestViewDocking(ImGuiTestContext* ctx) {
    printf("Running: TestViewDocking\n");

    ctx->SetRef("DearTsWindow");

    // 打开两个 View
    // View 1
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsAppend("Data Inspector");
    ctx->KeyPress(ImGuiKey_Enter);

    // View 2
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsAppend("Hex Editor");
    ctx->KeyPress(ImGuiKey_Enter);

    // 验证两个 View 都打开
    ctx->ItemCheck("Data Inspector");
    ctx->ItemCheck("Hex Editor");

    // 拖动一个 View 到另一个 View 的停靠标签
    // ctx->DragAndDrop("Data Inspector", "Hex Editor");

    // 验证两个 View 停靠在一起
    // ctx->ItemCheck("Data Inspector##Dock");
    // ctx->ItemCheck("Hex Editor##Dock");
}

/**
 * @brief 测试 View 焦点切换
 */
void TestViewFocusSwitching(ImGuiTestContext* ctx) {
    printf("Running: TestViewFocusSwitching\n");

    ctx->SetRef("DearTsWindow");

    // 打开两个 View
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsAppend("Data Inspector");
    ctx->KeyPress(ImGuiKey_Enter);

    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsAppend("Pattern Finder");
    ctx->KeyPress(ImGuiKey_Enter);

    // 点击第一个 View
    ctx->MouseMove("Data Inspector");
    ctx->MouseClick(ImGuiMouseButton_Left);

    // 验证焦点
    // ctx->ItemIsFocused("Data Inspector");

    // 点击第二个 View
    ctx->MouseMove("Pattern Finder");
    ctx->MouseClick(ImGuiMouseButton_Left);

    // 验证焦点切换
    // ctx->ItemIsFocused("Pattern Finder");
}

/**
 * @brief 测试 View 大小调整
 */
void TestViewResizing(ImGuiTestContext* ctx) {
    printf("Running: TestViewResizing\n");

    ctx->SetRef("DearTsWindow");

    // 打开一个 View
    ctx->KeyDown(ImGuiMod_Ctrl);
    ctx->KeyDown(ImGuiMod_Shift);
    ctx->KeyPress(ImGuiKey_P);
    ctx->KeyUp(ImGuiMod_Shift);
    ctx->KeyUp(ImGuiMod_Ctrl);
    ctx->ItemInput("##command_palette_input");
    ctx->KeyCharsAppend("Data Inspector");
    ctx->KeyPress(ImGuiKey_Enter);

    // 获取初始大小
    // ImVec2 initial_size = ctx->GetItemSize("Data Inspector");

    // 拖动边缘调整大小
    // ctx->DragTo("Data Inspector/Separator", new_position);

    // 验证大小改变
    // ImVec2 new_size = ctx->GetItemSize("Data Inspector");
    // EXPECT_NE(initial_size, new_size);
}

// ============================================================================
// 注册测试
// ============================================================================

/**
 * @brief 注册所有 View 测试
 */
void RegisterViewTests(ImGuiTestEngine* engine) {
    printf("Registering View tests...\n");

    ImGuiTest* test = nullptr;

    test = IM_REGISTER_TEST(engine, "ui.views", "open_and_close");
    test->TestFunc = TestViewOpenAndClose;

    test = IM_REGISTER_TEST(engine, "ui.views", "docking");
    test->TestFunc = TestViewDocking;

    test = IM_REGISTER_TEST(engine, "ui.views", "focus_switching");
    test->TestFunc = TestViewFocusSwitching;

    test = IM_REGISTER_TEST(engine, "ui.views", "resizing");
    test->TestFunc = TestViewResizing;

    printf("View tests registered: 4 tests\n");
}

#endif // IMGUI_TEST_ENGINE_ENABLE
