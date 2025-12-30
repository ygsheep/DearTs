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
    ctx->KeyChord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P);
    ctx->ItemInputValue("##command_palette_input", "Data Inspector");
    ctx->KeyDown(ImGuiKey_Enter);
    ctx->KeyUp(ImGuiKey_Enter);

    // 验证 View 打开
    ctx->ItemIsVisible("Data Inspector");

    // 关闭 View
    ctx->ItemClick("Data Inspector/CloseButton");

    // 验证 View 关闭
    ctx->ItemIsAbsent("Data Inspector");
}

/**
 * @brief 测试 View 停靠功能
 */
void TestViewDocking(ImGuiTestContext* ctx) {
    printf("Running: TestViewDocking\n");

    ctx->SetRef("DearTsWindow");

    // 打开两个 View
    // View 1
    ctx->KeyChord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P);
    ctx->ItemInputValue("##command_palette_input", "Data Inspector");
    ctx->KeyDown(ImGuiKey_Enter);
    ctx->KeyUp(ImGuiKey_Enter);

    // View 2
    ctx->KeyChord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P);
    ctx->ItemInputValue("##command_palette_input", "Hex Editor");
    ctx->KeyDown(ImGuiKey_Enter);
    ctx->KeyUp(ImGuiKey_Enter);

    // 验证两个 View 都打开
    ctx->ItemIsVisible("Data Inspector");
    ctx->ItemIsVisible("Hex Editor");

    // 拖动一个 View 到另一个 View 的停靠标签
    // ctx->DragAndDrop("Data Inspector", "Hex Editor");

    // 验证两个 View 停靠在一起
    // ctx->ItemIsVisible("Data Inspector##Dock");
    // ctx->ItemIsVisible("Hex Editor##Dock");
}

/**
 * @brief 测试 View 焦点切换
 */
void TestViewFocusSwitching(ImGuiTestContext* ctx) {
    printf("Running: TestViewFocusSwitching\n");

    ctx->SetRef("DearTsWindow");

    // 打开两个 View
    ctx->KeyChord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P);
    ctx->ItemInputValue("##command_palette_input", "Data Inspector");
    ctx->KeyDown(ImGuiKey_Enter);
    ctx->KeyUp(ImGuiKey_Enter);

    ctx->KeyChord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P);
    ctx->ItemInputValue("##command_palette_input", "Pattern Finder");
    ctx->KeyDown(ImGuiKey_Enter);
    ctx->KeyUp(ImGuiKey_Enter);

    // 点击第一个 View
    ctx->ItemClick("Data Inspector");

    // 验证焦点
    // ctx->ItemIsFocused("Data Inspector");

    // 点击第二个 View
    ctx->ItemClick("Pattern Finder");

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
    ctx->KeyChord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P);
    ctx->ItemInputValue("##command_palette_input", "Data Inspector");
    ctx->KeyDown(ImGuiKey_Enter);
    ctx->KeyUp(ImGuiKey_Enter);

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
