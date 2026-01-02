/**
 * @file main.cpp
 * @brief Toast Notification 示例程序
 * @details 展示如何使用 Toast Notification 插件和任务系统集成
 */

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <format>
#include <fstream>
#include <filesystem>

// 包含 Toast 管理器
#include "toast_manager.hpp"
#include "core/tasks/task_manager.h"
#include "core/event/event_bus.h"

using namespace DearTs::Plugins::Toast;
using namespace DearTs::Core::Tasks;
using namespace DearTs::Core::Event;

// 全局变量
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
bool g_running = true;

// 示例场景计数器
int g_scene = 0;
float g_timer = 0.0f;

// 模拟网络请求结果
bool g_network_success = true;

/**
 * @brief 初始化 SDL 和 ImGui
 */
bool init() {
    // 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    // 创建窗口
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Toast Notification 示例");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1280);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 720);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);

    g_window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    if (!g_window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }

    // 创建渲染器
    g_renderer = SDL_CreateRenderer(g_window, nullptr);
    if (!g_renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    // 初始化 ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // 加载中文字体
    io.Fonts->Clear();

    // 字体配置
    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    font_config.PixelSnapH = true;

    // 尝试加载字体（按优先级）
    static const char* font_paths[] = {
        "resources/fonts/OPPOSans-M.ttf",
        "resources/fonts/Noto nerd.ttf",
        "../resources/fonts/OPPOSans-M.ttf",
        "../../resources/fonts/OPPOSans-M.ttf"
    };

    bool font_loaded = false;
    for (const char* font_path : font_paths) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(
            font_path,
            16.0f,
            &font_config,
            io.Fonts->GetGlyphRangesChineseFull()
        );

        if (font != nullptr) {
            io.FontDefault = font;
            std::cout << "成功加载中文字体: " << font_path << std::endl;
            font_loaded = true;

            font_config.MergeMode = false;
            io.Fonts->AddFontFromFileTTF(font_path, 24.0f, &font_config, io.Fonts->GetGlyphRangesChineseFull());
            break;
        }
    }

    if (!font_loaded) {
        std::cerr << "警告：未能加载中文字体，使用默认字体" << std::endl;
        io.Fonts->AddFontDefault();
    }

    // 初始化 ImGui SDL3 后端
    ImGui_ImplSDL3_InitForSDLRenderer(g_window, g_renderer);
    ImGui_ImplSDLRenderer3_Init(g_renderer);

    // 设置样式
    ImGui::StyleColorsDark();

    // 订阅任务事件以显示 Toast 通知
    static auto completed_token = EventBus::instance().subscribe<TaskCompletedEvent>([](const TaskCompletedEvent& event) {
        std::string message = std::format("任务完成，耗时: {:.1f}ms", event.duration_ms);
        ToastManager::instance().success("任务完成", message);
    });

    static auto failed_token = EventBus::instance().subscribe<TaskFailedEvent>([](const TaskFailedEvent& event) {
        std::string message = std::format("错误: {}", event.error_message);
        ToastManager::instance().error("任务失败", message);
    });

    static auto cancelled_token = EventBus::instance().subscribe<TaskCancelledEvent>([](const TaskCancelledEvent& event) {
        std::string message = std::format("任务已取消，耗时: {:.1f}ms", event.duration_ms);
        ToastManager::instance().warning("任务取消", message);
    });

    (void)completed_token;
    (void)failed_token;
    (void)cancelled_token;

    std::cout << "初始化成功！" << std::endl;
    return true;
}

/**
 * @brief 清理资源
 */
void cleanup() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
    }

    if (g_window) {
        SDL_DestroyWindow(g_window);
    }

    SDL_Quit();
}

/**
 * @brief 渲染主界面
 */
void render_ui() {
    ImGui::Begin("Toast Notification 示例程序");

    ImGui::Text("欢迎使用 Toast Notification 示例！");
    ImGui::Separator();

    // 场景选择
    ImGui::Text("选择示例场景：");
    if (ImGui::Button("基本消息")) {
        g_scene = 1;
        g_timer = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("文件操作")) {
        g_scene = 2;
        g_timer = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("网络请求")) {
        g_scene = 3;
        g_timer = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("模型加载")) {
        g_scene = 4;
        g_timer = 0.0f;
    }
    if (ImGui::Button("关闭任务系统")) {
        ToastManager::instance().close_all();
    }

    std::string count_text = std::format("正在执行的任务总数: {}" , TaskManager::instance().getRunningTaskCount());

    ImGui::Text(count_text.c_str());

    ImGui::Spacing();
    ImGui::Separator();

    // 手动测试按钮
    ImGui::Text("手动测试：");
    if (ImGui::Button("显示信息")) {
        ToastManager::instance().info("信息", "这是一条信息提示");
    }
    ImGui::SameLine();
    if (ImGui::Button("显示成功")) {
        ToastManager::instance().success("成功", "操作已成功完成");
    }
    ImGui::SameLine();
    if (ImGui::Button("显示警告")) {
        ToastManager::instance().warning("警告", "请注意可能存在的问题");
    }
    ImGui::SameLine();
    if (ImGui::Button("显示错误")) {
        ToastManager::instance().error("错误", "操作失败，请重试");
    }

    ImGui::Spacing();
    if (ImGui::Button("关闭所有 Toast")) {
        ToastManager::instance().close_all();
    }

    // 统计信息
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("统计信息：");
    ImGui::BulletText("当前 Toast 数量: %zu", ToastManager::instance().get_count());
    ImGui::BulletText("运行中的任务数: %zu", TaskManager::instance().getRunningTaskCount());

    ImGui::End();
}

// ============================================================================
// 场景 1：基本消息
// ============================================================================

void run_scene_basic(float delta_time) {
    static float delay = 0.0f;
    delay += delta_time;

    if (delay > 1.0f) {
        delay = 0.0f;

        static int count = 0;
        if (count < 4) {
            switch (count) {
                case 0:
                    ToastManager::instance().info("提示", "欢迎使用 Toast Notification");
                    break;
                case 1:
                    ToastManager::instance().success("成功", "操作已完成");
                    break;
                case 2:
                    ToastManager::instance().warning("警告", "请注意此操作的影响");
                    break;
                case 3:
                    ToastManager::instance().error("错误", "发生了一个错误");
                    break;
            }
            count++;
        }
    }
}

// ============================================================================
// 场景 2：文件操作（使用任务系统）
// ============================================================================

void run_scene_file_operations(float delta_time) {
    static float delay = 0.0f;
    static int step = 0;
    static float last_task_time = 0.0f;  // 记录上次任务启动时间
    delay += delta_time;

    // 检查是否可以启动新任务（距离上次任务至少2秒）
    if (delay > 2.0f && (delay - last_task_time) > 2.0f) {
        delay = 0.0f;
        step++;
        last_task_time = 0.0f;  // 重置

        switch (step) {
            case 1: {
                // 文件读取任务
                auto task = TaskManager::instance().launch(
                    "读取文件",
                    [](const std::atomic<bool>& should_cancel) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        if (should_cancel) return;

                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        if (should_cancel) return;

                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    },
                    TaskType::Background
                );
                break;
            }
            case 2: {
                // 文件保存任务
                auto task = TaskManager::instance().launch(
                    "保存文件",
                    [](const std::atomic<bool>& should_cancel) {
                        for (int i = 0; i < 10 && !should_cancel; i++) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }
                    },
                    TaskType::Background
                );
                break;
            }
            case 3: {
                // 文件删除任务（模拟失败）
                auto task = TaskManager::instance().launch(
                    "删除文件",
                    [](const std::atomic<bool>& should_cancel) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        throw std::runtime_error("文件被占用，无法删除");
                    },
                    TaskType::Background
                );
                break;
            }
            case 4: {
                // 文件复制任务（模拟取消）
                auto task = TaskManager::instance().launch(
                    "复制文件",
                    [](const std::atomic<bool>& should_cancel) {
                        for (int i = 0; i < 20 && !should_cancel; i++) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }
                    },
                    TaskType::Background
                );
                break;
            }
            default:
                step = 0;
                break;
        }
    }
}

// ============================================================================
// 场景 3：网络请求（使用任务系统）
// ============================================================================

void run_scene_network_request(float delta_time) {
    static float delay = 0.0f;
    static int step = 0;
    static float last_task_time = 0.0f;
    delay += delta_time;

    if (delay > 2.0f && (delay - last_task_time) > 2.0f) {
        delay = 0.0f;
        step++;
        last_task_time = 0.0f;

        switch (step) {
            case 1: {
                // GET 请求任务
                auto task = TaskManager::instance().launch(
                    "GET 请求",
                    [](const std::atomic<bool>& should_cancel) {
                        for (int i = 0; i < 15 && !should_cancel; i++) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }
                    },
                    TaskType::Background
                );
                break;
            }
            case 2: {
                // POST 请求任务
                auto task = TaskManager::instance().launch(
                    "POST 请求",
                    [](const std::atomic<bool>& should_cancel) {
                        for (int i = 0; i < 12 && !should_cancel; i++) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }
                    },
                    TaskType::Background
                );
                break;
            }
            case 3: {
                // 上传文件任务（模拟失败）
                auto task = TaskManager::instance().launch(
                    "上传文件",
                    [](const std::atomic<bool>& should_cancel) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        throw std::runtime_error("网络连接超时");
                    },
                    TaskType::Background
                );
                break;
            }
            case 4: {
                // 下载文件任务
                auto task = TaskManager::instance().launch(
                    "下载文件",
                    [](const std::atomic<bool>& should_cancel) {
                        for (int i = 0; i < 20 && !should_cancel; i++) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }
                    },
                    TaskType::Background
                );
                break;
            }
            default:
                step = 0;
                break;
        }
    }
}

// ============================================================================
// 场景 4：模型加载（使用任务系统）
// ============================================================================

void run_scene_model_loading(float delta_time) {
    static float delay = 0.0f;
    static int step = 0;
    static float last_task_time = 0.0f;
    delay += delta_time;

    if (delay > 2.0f && (delay - last_task_time) > 2.0f) {
        delay = 0.0f;
        step++;
        last_task_time = 0.0f;

        switch (step) {
            case 1: {
                // 加载模型任务
                auto task = TaskManager::instance().launch(
                    "加载模型: Haru",
                    [](const std::atomic<bool>& should_cancel) {
                        // 模拟模型加载过程
                        for (int i = 0; i <= 100 && !should_cancel; i += 10) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }

                        if (!should_cancel) {
                            // 模拟加载成功
                        }
                    },
                    TaskType::Background
                );
                break;
            }
            case 2: {
                // 加载纹理任务
                auto task = TaskManager::instance().launch(
                    "加载纹理",
                    [](const std::atomic<bool>& should_cancel) {
                        for (int i = 0; i < 10 && !should_cancel; i++) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                    },
                    TaskType::Background
                );
                break;
            }
            case 3: {
                // 加载动画任务（模拟失败）
                auto task = TaskManager::instance().launch(
                    "加载动画",
                    [](const std::atomic<bool>& should_cancel) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        throw std::runtime_error("动画文件格式不支持");
                    },
                    TaskType::Background
                );
                break;
            }
            case 4: {
                // 加载物理任务（模拟取消）
                auto task = TaskManager::instance().launch(
                    "加载物理",
                    [](const std::atomic<bool>& should_cancel) {
                        for (int i = 0; i < 20 && !should_cancel; i++) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }
                    },
                    TaskType::Background
                );
                break;
            }
            default:
                step = 0;
                break;
        }
    }
}

// ============================================================================
// 主循环
// ============================================================================

void main_loop() {
    SDL_Event event;
    auto last_time = std::chrono::steady_clock::now();

    while (g_running) {
        // 计算 delta time
        auto current_time = std::chrono::steady_clock::now();
        float delta_time = std::chrono::duration<float>(current_time - last_time).count();
        last_time = current_time;

        // 处理事件
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) {
                g_running = false;
            }
        }

        // 处理异步事件
        EventBus::instance().process_async_events();

        // 更新 ToastManager
        ToastManager::instance().update(delta_time);

        // 开始 ImGui 帧
        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui::NewFrame();

        // 运行场景
        switch (g_scene) {
            case 1:
                run_scene_basic(delta_time);
                break;
            case 2:
                run_scene_file_operations(delta_time);
                break;
            case 3:
                run_scene_network_request(delta_time);
                break;
            case 4:
                run_scene_model_loading(delta_time);
                break;
        }

        // 渲染 UI
        render_ui();

        // 渲染 Toast
        ToastManager::instance().render();

        // 渲染 ImGui
        ImGui::Render();

        // 清屏
        SDL_SetRenderDrawColor(g_renderer, 30, 30, 30, 255);
        SDL_RenderClear(g_renderer);

        // 渲染 ImGui
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_renderer);
        SDL_RenderPresent(g_renderer);

        // 限制帧率
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Toast Notification 示例程序" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << std::endl;

    std::cout << "本示例展示任务系统与气泡插件的集成：" << std::endl;
    std::cout << "- 场景1：基本消息（直接调用 Toast API）" << std::endl;
    std::cout << "- 场景2：文件操作（使用任务系统）" << std::endl;
    std::cout << "- 场景3：网络请求（使用任务系统）" << std::endl;
    std::cout << "- 场景4：模型加载（使用任务系统）" << std::endl;
    std::cout << std::endl;

    // 初始化
    if (!init()) {
        std::cerr << "初始化失败！" << std::endl;
        return 1;
    }

    // 显示欢迎消息
    ToastManager::instance().success(
        "欢迎使用",
        "Toast Notification 示例程序已启动"
    );

    // 主循环
    main_loop();

    // 清理
    cleanup();

    std::cout << "程序正常退出" << std::endl;
    return 0;
}
