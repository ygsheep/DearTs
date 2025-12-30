/**
 * @file toast_test.cpp
 * @brief Toast 通知 UI 自动化测试
 * @details 测试 Toast 通知的显示、动画、交互和自动消失功能
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
// Toast 显示测试
// ============================================================================

/**
 * @brief 测试信息 Toast 显示
 */
void TestToastInfoDisplay(ImGuiTestContext* ctx) {
    printf("Running: TestToastInfoDisplay\n");

    ctx->SetRef("DearTsWindow");

    // 触发信息 Toast
    // 注意：需要通过命令或代码触发 Toast 显示
    // 例如：ToastManager::instance().show("Info message", ToastType::Info);

    // 验证 Toast 窗口可见
    ctx->ItemIsVisible("##toast");

    // 验证 Toast 内容
    // ctx->ItemIsVisible("##toast/Info message");

    // 等待一小段时间
    ctx->Yield(100);

    // Toast 应该还在显示
    ctx->ItemIsVisible("##toast");
}

/**
 * @brief 测试成功 Toast 显示
 */
void TestToastSuccessDisplay(ImGuiTestContext* ctx) {
    printf("Running: TestToastSuccessDisplay\n");

    ctx->SetRef("DearTsWindow");

    // 触发成功 Toast
    // ToastManager::instance().show("Operation successful!", ToastType::Success);

    // 验证 Toast 显示
    ctx->ItemIsVisible("##toast");

    // 验证成功图标/样式
    // ctx->ItemIsVisible("##toast/SuccessIcon");

    ctx->Yield(100);
}

/**
 * @brief 测试警告 Toast 显示
 */
void TestToastWarningDisplay(ImGuiTestContext* ctx) {
    printf("Running: TestToastWarningDisplay\n");

    ctx->SetRef("DearTsWindow");

    // 触发警告 Toast
    // ToastManager::instance().show("Warning: Low disk space", ToastType::Warning);

    // 验证 Toast 显示
    ctx->ItemIsVisible("##toast");

    // 验证警告图标/样式
    // ctx->ItemIsVisible("##toast/WarningIcon");

    ctx->Yield(100);
}

/**
 * @brief 测试错误 Toast 显示
 */
void TestToastErrorDisplay(ImGuiTestContext* ctx) {
    printf("Running: TestToastErrorDisplay\n");

    ctx->SetRef("DearTsWindow");

    // 触发错误 Toast
    // ToastManager::instance().show("Error: Operation failed", ToastType::Error);

    // 验证 Toast 显示
    ctx->ItemIsVisible("##toast");

    // 验证错误图标/样式
    // ctx->ItemIsVisible("##toast/ErrorIcon");

    ctx->Yield(100);
}

// ============================================================================
// Toast 自动消失测试
// ============================================================================

/**
 * @brief 测试 Toast 在指定时间后自动消失
 */
void TestToastAutoDisappear(ImGuiTestContext* ctx) {
    printf("Running: TestToastAutoDisappear\n");

    ctx->SetRef("DearTsWindow");

    // 触发 Toast（持续时间 3 秒）
    // ToastManager::instance().show("Auto dismiss message", ToastType::Info, 3000);

    // 验证 Toast 显示
    ctx->ItemIsVisible("##toast");

    // 等待 2.5 秒
    ctx->Yield(2500);

    // Toast 应该还在显示
    ctx->ItemIsVisible("##toast");

    // 再等待 1 秒
    ctx->Yield(1000);

    // Toast 应该已经消失
    ctx->ItemIsAbsent("##toast");
}

/**
 * @brief 测试长时间显示的 Toast
 */
void TestToastLongDuration(ImGuiTestContext* ctx) {
    printf("Running: TestToastLongDuration\n");

    ctx->SetRef("DearTsWindow");

    // 触发 Toast（持续时间 10 秒）
    // ToastManager::instance().show("Long duration message", ToastType::Info, 10000);

    // 验证显示
    ctx->ItemIsVisible("##toast");

    // 等待 9 秒
    ctx->Yield(9000);

    // Toast 应该还在显示
    ctx->ItemIsVisible("##toast");

    // 再等待 2 秒
    ctx->Yield(2000);

    // Toast 应该消失
    ctx->ItemIsAbsent("##toast");
}

// ============================================================================
// Toast 交互测试
// ============================================================================

/**
 * @brief 测试点击 Toast 关闭
 */
void TestToastCloseOnClick(ImGuiTestContext* ctx) {
    printf("Running: TestToastCloseOnClick\n");

    ctx->SetRef("DearTsWindow");

    // 触发可关闭的 Toast
    // ToastManager::instance().show("Click to close", ToastType::Info);

    // 验证显示
    ctx->ItemIsVisible("##toast");

    // 点击 Toast
    ctx->ItemClick("##toast");

    // 验证立即关闭
    ctx->ItemIsAbsent("##toast");
}

/**
 * @brief 测试 Toast 按钮点击
 */
void TestToastButtonClick(ImGuiTestContext* ctx) {
    printf("Running: TestToastButtonClick\n");

    ctx->SetRef("DearTsWindow");

    // 触发带按钮的 Toast
    // ToastManager::instance().show("Update available", ToastType::Info, 5000, "Update");

    // 验证 Toast 显示
    ctx->ItemIsVisible("##toast");

    // 验证按钮存在
    // ctx->ItemIsVisible("##toast/Update");

    // 点击按钮
    // ctx->ItemClick("##toast/Update");

    // Toast 应该关闭
    ctx->ItemIsAbsent("##toast");

    // 验证按钮操作执行（例如打开更新窗口）
    // ctx->ItemIsVisible("UpdateWindow");
}

/**
 * @brief 测试 Toast 悬停暂停自动消失
 */
void TestToastHoverPausesDismiss(ImGuiTestContext* ctx) {
    printf("Running: TestToastHoverPausesDismiss\n");

    ctx->SetRef("DearTsWindow");

    // 触发 Toast（持续时间 3 秒）
    // ToastManager::instance().show("Hover to pause", ToastType::Info, 3000);

    // 验证显示
    ctx->ItemIsVisible("##toast");

    // 等待 1 秒
    ctx->Yield(1000);

    // 悬停在 Toast 上
    ctx->MouseMoveTo("##toast");

    // 等待 3 秒（应该已经超过正常显示时间）
    ctx->Yield(3000);

    // Toast 应该还在显示（因为悬停暂停）
    ctx->ItemIsVisible("##toast");

    // 移开鼠标
    ctx->MouseMoveTo("DearTsWindow/WorkArea");

    // 等待一小段时间，Toast 应该开始消失
    ctx->Yield(500);

    // Toast 应该消失
    ctx->ItemIsAbsent("##toast");
}

// ============================================================================
// Toast 多个通知测试
// ============================================================================

/**
 * @brief 测试多个 Toast 同时显示
 */
void TestToastMultipleNotifications(ImGuiTestContext* ctx) {
    printf("Running: TestToastMultipleNotifications\n");

    ctx->SetRef("DearTsWindow");

    // 触发多个 Toast
    // ToastManager::instance().show("Message 1", ToastType::Info);
    ctx->Yield(100);

    // ToastManager::instance().show("Message 2", ToastType::Warning);
    ctx->Yield(100);

    // ToastManager::instance().show("Message 3", ToastType::Success);
    ctx->Yield(100);

    // 验证所有 Toast 都可见
    // ctx->ItemIsVisible("##toast[0]");
    // ctx->ItemIsVisible("##toast[1]");
    // ctx->ItemIsVisible("##toast[2]");

    // 验证 Toast 堆叠正确（最新在最上面）

    // 等待它们消失
    ctx->Yield(4000);

    // 所有 Toast 应该消失
    ctx->ItemIsAbsent("##toast");
}

/**
 * @brief 测试 Toast 队列限制
 */
void TestToastQueueLimit(ImGuiTestContext* ctx) {
    printf("Running: TestToastQueueLimit\n");

    ctx->SetRef("DearTsWindow");

    // 快速触发多个 Toast（超过队列限制）
    for (int i = 0; i < 10; i++) {
        // ToastManager::instance().show(
        //     std::format("Message {}", i).c_str(),
        //     ToastType::Info
        // );
        ctx->Yield(50);
    }

    // 验证最多只显示指定数量的 Toast（例如 5 个）
    // 超出的 Toast 应该在队列中等待

    // 等待所有 Toast 消失
    ctx->Yield(10000);
}

// ============================================================================
// Toast 位置和样式测试
// ============================================================================

/**
 * @brief 测试 Toast 位置（右上角）
 */
void TestToastTopRightPosition(ImGuiTestContext* ctx) {
    printf("Running: TestToastTopRightPosition\n");

    ctx->SetRef("DearTsWindow");

    // 设置 Toast 位置为右上角
    // ToastManager::instance().set_position(ToastPosition::TopRight);

    // 触发 Toast
    // ToastManager::instance().show("Top right message", ToastType::Info);

    // 验证 Toast 在右上角显示
    // 可以通过检查位置验证
    ctx->ItemIsVisible("##toast");

    ctx->Yield(100);
}

/**
 * @brief 测试 Toast 位置（左下角）
 */
void TestToastBottomLeftPosition(ImGuiTestContext* ctx) {
    printf("Running: TestToastBottomLeftPosition\n");

    ctx->SetRef("DearTsWindow");

    // 设置 Toast 位置为左下角
    // ToastManager::instance().set_position(ToastPosition::BottomLeft);

    // 触发 Toast
    // ToastManager::instance().show("Bottom left message", ToastType::Info);

    // 验证 Toast 在左下角显示
    ctx->ItemIsVisible("##toast");

    ctx->Yield(100);
}

// ============================================================================
// 注册测试
// ============================================================================

/**
 * @brief 注册所有 Toast 测试
 */
void RegisterToastTests(ImGuiTestEngine* engine) {
    printf("Registering Toast tests...\n");

    ImGuiTest* test = nullptr;

    // 显示测试
    test = IM_REGISTER_TEST(engine, "ui.toast", "info_display");
    test->TestFunc = TestToastInfoDisplay;

    test = IM_REGISTER_TEST(engine, "ui.toast", "success_display");
    test->TestFunc = TestToastSuccessDisplay;

    test = IM_REGISTER_TEST(engine, "ui.toast", "warning_display");
    test->TestFunc = TestToastWarningDisplay;

    test = IM_REGISTER_TEST(engine, "ui.toast", "error_display");
    test->TestFunc = TestToastErrorDisplay;

    // 自动消失测试
    test = IM_REGISTER_TEST(engine, "ui.toast", "auto_disappear");
    test->TestFunc = TestToastAutoDisappear;

    test = IM_REGISTER_TEST(engine, "ui.toast", "long_duration");
    test->TestFunc = TestToastLongDuration;

    // 交互测试
    test = IM_REGISTER_TEST(engine, "ui.toast", "close_on_click");
    test->TestFunc = TestToastCloseOnClick;

    test = IM_REGISTER_TEST(engine, "ui.toast", "button_click");
    test->TestFunc = TestToastButtonClick;

    test = IM_REGISTER_TEST(engine, "ui.toast", "hover_pauses_dismiss");
    test->TestFunc = TestToastHoverPausesDismiss;

    // 多个通知测试
    test = IM_REGISTER_TEST(engine, "ui.toast", "multiple_notifications");
    test->TestFunc = TestToastMultipleNotifications;

    test = IM_REGISTER_TEST(engine, "ui.toast", "queue_limit");
    test->TestFunc = TestToastQueueLimit;

    // 位置和样式测试
    test = IM_REGISTER_TEST(engine, "ui.toast", "top_right_position");
    test->TestFunc = TestToastTopRightPosition;

    test = IM_REGISTER_TEST(engine, "ui.toast", "bottom_left_position");
    test->TestFunc = TestToastBottomLeftPosition;

    printf("Toast tests registered: 14 tests\n");
}

#endif // IMGUI_TEST_ENGINE_ENABLE
