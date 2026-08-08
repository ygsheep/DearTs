/**
 * @file main.cpp
 * @brief WhaleDock 灵动岛 GUI Theme 验证示例
 * @details
 *   使用 SDL3_Renderer + ImGui 渲染 whale_dock::IslandRenderer，
 *   通过左侧控制面板切换状态机/数据，实时验证 theme.hpp 的视觉效果。
 *
 *   操作：
 *     1/2/3/4  切换到 Collapsed / Expanded / Alert / Hidden
 *     T        触发 Thinking Start
 *     Y        触发 Thinking Done
 *     B        触发 ToolCall(Bash)
 *     R        触发 ToolResult(成功)
 *     E        触发 ToolResult(失败)
 *     Esc      退出
 *     鼠标点击岛体 = onClick()
 */

#include "island.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>

// ---- 全局状态 ------------------------------------------------------------

static SDL_Window*   g_window   = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static bool          g_running  = true;

static whale_dock::IslandController g_ctrl;
static whale_dock::IslandRenderer   g_islandRenderer;

// 岛在窗口中的位置（左上角）
static constexpr float kIslandX = 60.0f;
static constexpr float kIslandY = 30.0f;

// 演示用 tool_call 计数
static int g_toolCallSeq = 0;

// ---- 辅助 ----------------------------------------------------------------

static void LogF(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}

/// 触发一次工具调用
static void TriggerToolCall() {
    const char* tools[] = {"Bash", "Read", "Edit", "Glob", "Grep"};
    const char* args[]  = {
        "cmake --build build --config Release",
        "src/main.cpp",
        "src/utils.cpp",
        "**/*.hpp",
        "TODO|FIXME",
    };
    const int idx = g_toolCallSeq % 5;
    ++g_toolCallSeq;
    g_ctrl.onToolCall(tools[idx], args[idx]);
}

/// 触发最近一次工具调用的结果
static void TriggerToolResult(bool isError) {
    const auto& history = g_ctrl.data().tool_history;
    if (history.empty()) {
        LogF("没有可结束的工具调用");
        return;
    }
    const int idx = static_cast<int>(history.size()) - 1;
    const float dur = 800.0f + (idx % 4) * 600.0f;
    g_ctrl.onToolResult(idx, isError, dur,
                        isError ? "exit code 1, 编译失败"
                                : "exit code 0, 构建成功（42 目标）");
}

// ---- 初始化 --------------------------------------------------------------

static bool Init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LogF("SDL_Init 失败: %s", SDL_GetError());
        return false;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                          "WhaleDock Theme 验证");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 500);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 460);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    g_window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!g_window) {
        LogF("窗口创建失败: %s", SDL_GetError());
        return false;
    }

    // SDL3_Renderer：跨平台一份代码，自动选择 D3D11/Metal/Vulkan 后端
    g_renderer = SDL_CreateRenderer(g_window, nullptr);
    if (!g_renderer) {
        LogF("渲染器创建失败: %s", SDL_GetError());
        return false;
    }

    // ---- ImGui 初始化 ----
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 中文字体：OPPOSans-M（16 默认 / 12 小字 / 20 大字）
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
        cfg.PixelSnapH  = true;
        const ImWchar* ranges = io.Fonts->GetGlyphRangesChineseFull();
        io.Fonts->AddFontFromFileTTF(font_path, 16.0f, &cfg, ranges);  // Fonts[0] 默认
        io.Fonts->AddFontFromFileTTF(font_path, 12.0f, &cfg, ranges);  // Fonts[1] 小字
        io.Fonts->AddFontFromFileTTF(font_path, 20.0f, &cfg, ranges);  // Fonts[2] 大字
        LogF("字体已加载: %s", font_path);
    } else {
        LogF("警告: 未找到中文字体，使用默认字体");
        io.Fonts->AddFontDefault();
    }

    ImGui_ImplSDL3_InitForSDLRenderer(g_window, g_renderer);
    ImGui_ImplSDLRenderer3_Init(g_renderer);

    // 深色主题，但窗口背景由 SDL_RenderClear 控制
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding  = 4.0f;

    // 初始进入 Collapsed
    g_ctrl.transitionTo(whale_dock::Phase::Collapsed);
    return true;
}

static void Cleanup() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window)   SDL_DestroyWindow(g_window);
    SDL_Quit();
}

// ---- 控制面板 ------------------------------------------------------------

static void DrawControlPanel() {
    ImGui::SetNextWindowPos(ImVec2(20, 230), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(460, 210), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("控制面板", nullptr,
                      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove)) {
        ImGui::End();
        return;
    }

    // ---- 状态切换 ----
    ImGui::TextDisabled("状态切换");
    ImGui::Separator();
    const char* phase_names[] = {"Hidden", "Collapsed", "Expanded", "Alert"};
    const whale_dock::Phase phases[] = {
        whale_dock::Phase::Hidden,
        whale_dock::Phase::Collapsed,
        whale_dock::Phase::Expanded,
        whale_dock::Phase::Alert,
    };
    for (int i = 0; i < 4; ++i) {
        if (i > 0) ImGui::SameLine();
        const bool active = g_ctrl.phase() == phases[i];
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.55f, 0.85f, 1.0f));
        if (ImGui::Button(phase_names[i], ImVec2(90, 0))) {
            g_ctrl.transitionTo(phases[i]);
        }
        if (active) ImGui::PopStyleColor();
    }

    // ---- 事件触发 ----
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::TextDisabled("事件触发");
    ImGui::Separator();
    if (ImGui::Button("思考开始##tstart", ImVec2(100, 0))) g_ctrl.onThinkingStart();
    ImGui::SameLine();
    if (ImGui::Button("思考结束##tdone", ImVec2(100, 0))) g_ctrl.onThinkingDone(2.3f);
    ImGui::SameLine();
    if (ImGui::Button("工具调用##tcall", ImVec2(100, 0))) TriggerToolCall();

    if (ImGui::Button("结果(成功)##ok", ImVec2(100, 0))) TriggerToolResult(false);
    ImGui::SameLine();
    if (ImGui::Button("结果(失败)##err", ImVec2(100, 0))) TriggerToolResult(true);
    ImGui::SameLine();
    if (ImGui::Button("告警##alert", ImVec2(100, 0))) g_ctrl.onAlert("演示告警：余额不足");

    // ---- 数据调整 ----
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::TextDisabled("数据调整");
    ImGui::Separator();
    auto& data = g_ctrl.data();

    float balance = static_cast<float>(data.balance_usd);
    if (ImGui::SliderFloat("余额 (USD)", &balance, 0.0f, 20.0f, "%.3f")) {
        g_ctrl.onBalanceUpdate(balance);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", balance < 1.0f ? "红" : (balance < 5.0f ? "黄" : "绿"));

    float cost = static_cast<float>(data.task_cost);
    if (ImGui::SliderFloat("本任务花费", &cost, 0.0f, 0.5f, "%.4f")) {
        g_ctrl.onCostUpdate(cost);
    }

    int ctxUsed = data.context_used;
    if (ImGui::SliderInt("上下文已用", &ctxUsed, 0, data.context_total)) {
        data.context_used = ctxUsed;
    }

    int cacheHit = data.cache_hit_rate;
    if (ImGui::SliderInt("缓存命中 %%", &cacheHit, 0, 100)) {
        data.cache_hit_rate = cacheHit;
    }

    ImGui::Checkbox("花费为估算", &data.cost_estimated);

    ImGui::End();
}

// ---- 顶部提示条 ----------------------------------------------------------

static void DrawHintBar(float width) {
    ImGui::SetNextWindowPos(ImVec2(kIslandX, kIslandY + whale_dock::theme::kExpandedH + 10),
                            ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.16f, 0.85f));
    ImGui::Begin("HintBar", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextDisabled("提示:");
    ImGui::SameLine();
    ImGui::TextUnformatted("1/2/3/4 切换状态 · T/Y 思考 · B/R/E 工具 · 点击岛锁定/收起 · Esc 退出");
    ImGui::End();
    ImGui::PopStyleColor();
}

// ---- 主循环 --------------------------------------------------------------

int main(int, char**) {
    if (!Init()) return 1;

    auto last = std::chrono::steady_clock::now();

    while (g_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                g_running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_ESCAPE: g_running = false; break;
                    case SDLK_1: g_ctrl.transitionTo(whale_dock::Phase::Hidden);    break;
                    case SDLK_2: g_ctrl.transitionTo(whale_dock::Phase::Collapsed); break;
                    case SDLK_3: g_ctrl.transitionTo(whale_dock::Phase::Expanded);  break;
                    case SDLK_4: g_ctrl.transitionTo(whale_dock::Phase::Alert);     break;
                    case SDLK_T: g_ctrl.onThinkingStart();              break;
                    case SDLK_Y: g_ctrl.onThinkingDone(2.3f);           break;
                    case SDLK_B: TriggerToolCall();                     break;
                    case SDLK_R: TriggerToolResult(false);              break;
                    case SDLK_E: TriggerToolResult(true);               break;
                    default: break;
                }
            }
        }

        // 帧间隔
        const auto now  = std::chrono::steady_clock::now();
        const float dt  = std::chrono::duration<float>(now - last).count();
        last = now;

        // 鼠标悬停检测（传给 controller 用于自动收起逻辑）
        const ImVec2 mouse = ImGui::GetMousePos();
        const float  w     = whale_dock::theme::kWidth;
        const float  h     = g_ctrl.currentHeight();
        const bool   hover = (mouse.x >= kIslandX && mouse.x <= kIslandX + w &&
                              mouse.y >= kIslandY && mouse.y <= kIslandY + h);

        g_ctrl.update(dt, hover);

        // ---- ImGui 帧 ----
        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui::NewFrame();

        // 顶部岛（用 BackgroundDrawList，确保在所有窗口之下）
        g_islandRenderer.draw(ImGui::GetBackgroundDrawList(),
                              kIslandX, kIslandY, g_ctrl);
        if (g_islandRenderer.consumeClick()) {
            g_ctrl.onClick();
        }

        // 顶部提示条（紧跟在岛最大高度下方）
        DrawHintBar(whale_dock::theme::kWidth);

        // 控制面板
        DrawControlPanel();

        ImGui::Render();

        // ---- SDL 渲染 ----
        // 深色背景，模拟桌面环境
        SDL_SetRenderDrawColorFloat(g_renderer, 0.08f, 0.08f, 0.10f, 1.0f);
        SDL_RenderClear(g_renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_renderer);
        SDL_RenderPresent(g_renderer);
    }

    Cleanup();
    return 0;
}
