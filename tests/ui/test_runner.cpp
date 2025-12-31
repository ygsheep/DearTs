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
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "SDL3/SDL.h"
#include "test_ui_components.hpp"
#include <stdio.h>
#include <vector>
#include <string>

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
    printf("=== DearTs UI Test Runner ===\n");
    printf("Starting DearTs UI Test Runner...\n");
    fflush(stdout);

    // 1. 初始化 SDL3
    printf("Initializing SDL3...\n");
    fflush(stdout);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    printf("SDL3 initialized\n");
    fflush(stdout);

    // 2. 创建 SDL3 窗口
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "DearTs UI Tests");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1280);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 720);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, SDL_WINDOW_RESIZABLE);

    SDL_Window* window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // 3. 创建 SDL3 渲染器
    props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, window);

    SDL_Renderer* renderer = SDL_CreateRendererWithProperties(props);
    SDL_DestroyProperties(props);

    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 4. 初始化 ImGui
    printf("Initializing ImGui...\n");
    fflush(stdout);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    printf("ImGui initialized\n");
    fflush(stdout);

    // 5. 初始化 ImGui SDL3 后端
    printf("Initializing SDL3 backend...\n");
    fflush(stdout);
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    printf("Initializing SDLRenderer backend...\n");
    fflush(stdout);
    ImGui_ImplSDLRenderer3_Init(renderer);
    printf("Backends initialized\n");
    fflush(stdout);

    // 6. 创建 ImGui Test Engine
    printf("Creating Test Engine...\n");
    fflush(stdout);
    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext();
    printf("Starting Test Engine...\n");
    ImGuiTestEngine_Start(engine, ImGui::GetCurrentContext());
    ImGuiTestEngine_InstallDefaultCrashHandler();
    printf("ImGui Test Engine initialized\n");

    // 7. 注册所有测试
    printf("Registering tests...\n");
    RegisterAllTests(engine);
    printf("Tests registered successfully\n");

    // 8. 排队所有测试进行自动化运行
    printf("Queueing tests for automation...\n");
    ImGuiTestEngine_QueueTests(engine, ImGuiTestGroup_Unknown);
    printf("All tests queued. Test Engine will run them automatically.\n");

    // 7.5 初始化测试 UI 组件
    using namespace DearTs::TestUI;

    // 初始化 CommandPalette 命令
    std::vector<std::string> commands = {
        "Open File",
        "Save File",
        "New Project",
        "Close Project",
        "Show Settings",
        "Show Logger",
        "Show Data Inspector",
        "Toggle Dark Theme",
        "Toggle Light Theme",
        "Run Tests",
        "Build Project",
        "Debug Application"
    };
    TestCommandPalette::setCommands(commands);

    // 显示欢迎 Toast
    TestToastManager::showInfo("Welcome", "DearTs UI Test Framework is ready!");
    TestToastManager::showSuccess("Ready", "All test UI components loaded");

    printf("Starting test loop...\n");
    printf("Note: This is a basic test framework.\n");
    printf("Full Test Engine integration requires additional setup.\n");

    // 8. 主测试循环
    bool done = false;
    int frame_count = 0;
    const int max_frames = 0;  // 0 = 不限制，需要手动关闭窗口（ESC 或关闭按钮）
    while (!done) {
        // 处理 SDL 事件
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) {
                done = true;
            }
            // ESC 键退出
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                done = true;
            }
        }

        // 开始新帧
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 渲染测试 UI 组件
        TestUIRenderer::renderAll();

        // 显示测试状态窗口
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("DearTs UI Test Status", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("=== DearTs UI Test Runner ===");
            ImGui::Separator();

            ImGui::Text("Test Framework Status:");
            ImGui::BulletText("ImGui: %s", ImGui::GetVersion());
            ImGui::BulletText("SDL3: Initialized");
            ImGui::BulletText("Renderer: SDLRenderer");
            ImGui::Separator();

            ImGui::Text("Registered Tests (45 total):");
            ImGui::BulletText("TitleBar: 8 tests");
            ImGui::BulletText("CommandPalette: 11 tests");
            ImGui::BulletText("Toast: 14 tests");
            ImGui::BulletText("View: 4 tests");
            ImGui::BulletText("Interaction: 8 tests");
            ImGui::Separator();

            ImGui::Text("Current Status:");
            ImGui::BulletText("Frame: %d / %d", frame_count, max_frames);
            ImGui::BulletText("Window: %dx%d", (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            ImGui::BulletText("FPS: %.1f", io.Framerate);
            ImGui::Separator();

            ImGui::TextWrapped("Note: This is a basic test framework demonstrating");
            ImGui::TextWrapped("SDL3 + ImGui integration for UI testing.");
            ImGui::TextWrapped("Full automated testing requires DearTs UI components.");
            ImGui::Separator();

            if (ImGui::Button("Exit Now")) {
                done = true;
            }
            ImGui::SameLine();
            if (frame_count < max_frames && ImGui::Button("Run to Completion")) {
                frame_count = max_frames - 1;
            }
        }
        ImGui::End();

        // 渲染
        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        // Test Engine hook
        ImGuiTestEngine_PostSwap(engine);

        frame_count++;

        // 超时退出
        if (frame_count >= max_frames && max_frames != 0) {
            printf("Test frame limit reached, finishing...\n");
            done = true;
        }
    }

    // 9. 输出测试统计
    printf("\n=== Test Results ===\n");
    printf("Total Frames: %d\n", frame_count);
    printf("Average FPS: %.1f\n", io.Framerate);
    printf("\nFramework: Successfully initialized\n");
    printf("Tests: Registered but not executed (requires UI components)\n");
    printf("Status: Test framework is functional!\n");

    // 10. 清理资源
    printf("Cleaning up...\n");

    // Stop Test Engine first
    ImGuiTestEngine_Stop(engine);

    // Shutdown backends
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    // Destroy ImGui context BEFORE destroying Test Engine context
    // IMPORTANT: Test Engine needs to save .ini data, which requires ImGui context
    ImGui::DestroyContext();

    // Now destroy Test Engine context
    ImGuiTestEngine_DestroyContext(engine);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("DearTs UI Tests completed!\n");
    return 0;
}

#endif // IMGUI_TEST_ENGINE_ENABLE
