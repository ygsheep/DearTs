/**
 * @file main.cpp
 * @brief 消息灵动岛原型（DearTs 宿主版）：SDL3 + ImGui + workx EventBus 桥
 * @details 主窗口内右上角 overlay 渲染 macOS 风格灵动岛通知。
 *          workx 以 add_subdirectory 方式嵌入（外部工程消费模式），
 *          演示事件走真实 workx EventBus -> EventBridge 通道。
 *          操作：Esc 退出；F2 注入演示通知；点击岛 = 展开/收起 + 重置停留计时。
 */

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

#include "core/events/agent_events.h"
#include "core/events/event_bus.h"

#include "dynamic_island.h"
#include "event_bridge.h"

// ---- 全局状态 ------------------------------------------------------------

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static bool g_running = true;

static di::DynamicIsland g_island;
static di::EventBridge* g_bridge = nullptr;
static int g_demo_step = 0;
static std::mt19937 g_rng{0x5EED};

static const char* kDemoTitles[] = {
    "工具调用: Bash", "工具调用: ReadFile", "工具调用: Glob",
    "上下文压缩已恢复", "上下文压缩已暂停",
};

// ---- 演示辅助 -------------------------------------------------------------

static void LogF(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}

static void InjectDemoNotification() {
    static std::uniform_int_distribution<int> kind_dist(0, 3);
    const auto kind = static_cast<di::NotifyKind>(kind_dist(g_rng));
    const char* title = kDemoTitles[g_demo_step % 5];
    const char* body =
        "这是一条动态岛演示通知：验证进入、展开与自动消失动画";
    g_island.Push(kind, title, body, 4.0f);
    ++g_demo_step;
}

/// 每帧模拟一轮 agent 事件（走真实 EventBus -> EventBridge 通道）
static void PublishDemoEvents(agent::IEventBus& bus) {
    static int call_no = 0;
    switch (g_demo_step % 4) {
    case 0:
        bus.publish(agent::ToolCallEvent{
            .tool_name = "Bash",
            .arguments = "cmake --build build --config Debug",
            .call_id = "call_" + std::to_string(call_no++),
            .tool_type = agent::tool::ToolType::Execute,
        });
        break;
    case 1:
        bus.publish(agent::ToolResultEvent{
            .call_id = "call_" + std::to_string(call_no - 1),
            .result = "exit code 0，构建成功（42 个目标）",
            .is_error = false,
        });
        break;
    case 2:
        bus.publish(agent::AgentDoneEvent{
            .final_response = "已完成库化改造的构建验证，全部目标编译通过",
            .total_steps = 3,
            .total_tool_calls = 2,
            .total_duration_ms = 1234.5,
        });
        break;
    default:
        bus.publish(agent::CompactionPausedEvent{
            .session_id = "demo",
            .paused = true,
            .consecutive_compacts = 3,
            .tokens_before = 42000,
            .ratio = 0.93f,
            .notice = "连续 3 次 compact 未降低占用比，触发保护性暂停压缩",
        });
        break;
    }
    ++g_demo_step;
}

// ---- 初始化 ---------------------------------------------------------------

static bool init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LogF("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                          "Dynamic Island 示例（DearTs + workx）");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1280);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 720);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    g_window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!g_window) {
        LogF("Failed to create window: %s", SDL_GetError());
        return false;
    }

    g_renderer = SDL_CreateRenderer(g_window, nullptr);
    if (!g_renderer) {
        LogF("Failed to create renderer: %s", SDL_GetError());
        return false;
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 中文字体：OPPOSans-M（16 默认 / 12 正文 / 24 标题）
    static const char* font_paths[] = {
        "resources/fonts/OPPOSans-M.ttf",
        "../resources/fonts/OPPOSans-M.ttf",
        "../../resources/fonts/OPPOSans-M.ttf",
    };
    const char* font_path = nullptr;
    for (const char* p : font_paths) {
        if (std::filesystem::exists(p)) {
            font_path = p;
            break;
        }
    }

    if (font_path) {
        ImFontConfig cfg;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        cfg.PixelSnapH = true;
        const ImWchar* ranges = io.Fonts->GetGlyphRangesChineseFull();
        io.Fonts->AddFontFromFileTTF(font_path, 16.0f, &cfg, ranges); // Fonts[0] 默认
        io.Fonts->AddFontFromFileTTF(font_path, 12.0f, &cfg, ranges); // Fonts[1] 正文
        io.Fonts->AddFontFromFileTTF(font_path, 24.0f, &cfg, ranges); // Fonts[2] 标题
        LogF("字体已加载: %s", font_path);
    } else {
        LogF("警告: 未找到中文字体，使用默认字体");
        io.Fonts->AddFontDefault();
    }

    ImGui_ImplSDL3_InitForSDLRenderer(g_window, g_renderer);
    ImGui_ImplSDLRenderer3_Init(g_renderer);
    ImGui::StyleColorsDark();

    // workx EventBus 桥（同进程直链 workx_core，宿主无关事件协议）
    g_bridge = new di::EventBridge(agent::EventBus::instance());
    LogF("初始化完成，workx EventBus 桥已建立");
    return true;
}

static void cleanup() {
    delete g_bridge;
    g_bridge = nullptr;

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);
    SDL_Quit();
}

// ---- 主循环 ---------------------------------------------------------------

int main(int, char**) {
    if (!init()) return 1;

    std::vector<di::IslandMessage> messages;
    auto last_demo = std::chrono::steady_clock::now();
    const float kIslandWidth = 360.0f;
    const float kMargin = 16.0f;

    while (g_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                g_running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) g_running = false;
                if (event.key.key == SDLK_F2) InjectDemoNotification();
            }
        }

        // 每 3 秒发布一轮演示事件（真实 EventBus -> bridge -> 岛）
        const auto now = std::chrono::steady_clock::now();
        if (now - last_demo >= std::chrono::seconds(3)) {
            PublishDemoEvents(agent::EventBus::instance());
            last_demo = now;
        }

        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();

        // 演示控制面板
        ImGui::Begin("Dynamic Island 演示");
        ImGui::TextWrapped("workx EventBus 事件每 3 秒自动发布一轮；也可以手动操作：");
        if (ImGui::Button("注入演示通知")) InjectDemoNotification();
        ImGui::SameLine();
        if (ImGui::Button("清空通知")) g_island.Clear();
        ImGui::SameLine();
        if (ImGui::Button("退出")) g_running = false;
        ImGui::Separator();
        ImGui::TextWrapped("提示: 点击右上角灵动岛可展开/收起；F2 快捷注入；Esc 退出。");
        ImGui::End();

        // 桥接消息 -> 岛
        if (g_bridge) {
            g_bridge->Drain(messages);
            for (const auto& m : messages) {
                g_island.Push(m.kind, m.title, m.body);
            }
            messages.clear();
        }

        // 右上角 overlay
        g_island.Update(io.DeltaTime);
        const ImVec2 pos(io.DisplaySize.x - kMargin - kIslandWidth, kMargin);
        g_island.Draw(ImGui::GetBackgroundDrawList(), pos.x, pos.y, kIslandWidth);

        ImGui::Render();
        SDL_SetRenderDrawColorFloat(g_renderer, 0.10f, 0.10f, 0.12f, 1.0f);
        SDL_RenderClear(g_renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_renderer);
        SDL_RenderPresent(g_renderer);
    }

    cleanup();
    return 0;
}
