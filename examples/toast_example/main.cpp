/**
 * @file main.cpp
 * @brief Toast Notification 示例程序
 * @details 展示如何使用 Toast Notification 插件
 */

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include <thread>

// 包含 Toast 管理器
#include "toast_manager.hpp"

using namespace DearTs::Plugins::Toast;

// 全局变量
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
bool g_running = true;

// 示例场景计数器
int g_scene = 0;
float g_timer = 0.0f;

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
    io.Fonts->Clear(); // 清除默认字体

    // 字体配置
    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    font_config.PixelSnapH = true;

    // 尝试加载字体（按优先级）
    static const char* font_paths[] = {
        "resources/fonts/OPPOSans-M.ttf",      // 运行目录
        "resources/fonts/Noto nerd.ttf",       // 运行目录
        "../resources/fonts/OPPOSans-M.ttf",  // IDE 调试目录
        "../../resources/fonts/OPPOSans-M.ttf" // 更深的调试目录
    };

    bool font_loaded = false;
    for (const char* font_path : font_paths) {
        // 加载中文字体，包含完整的汉字字符集
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

            // 添加更大的字体用于标题
            font_config.MergeMode = false; // 不合并，创建独立的字体
            io.Fonts->AddFontFromFileTTF(font_path, 24.0f, &font_config, io.Fonts->GetGlyphRangesChineseFull());
            break;
        }
    }

    if (!font_loaded) {
        std::cerr << "警告：未能加载中文字体，使用默认字体（可能无法显示中文）" << std::endl;
        // 使用默认字体作为后备
        io.Fonts->AddFontDefault();
    }

    // 初始化 ImGui SDL3 后端
    ImGui_ImplSDL3_InitForSDLRenderer(g_window, g_renderer);
    ImGui_ImplSDLRenderer3_Init(g_renderer);

    // 设置样式
    ImGui::StyleColorsDark();

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
    if (ImGui::Button("表单验证")) {
        g_scene = 3;
        g_timer = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("进度反馈")) {
        g_scene = 4;
        g_timer = 0.0f;
    }

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

    ImGui::End();
}

/**
 * @brief 运行场景1：基本消息
 */
void run_scene_basic(float delta_time) {
    static float delay = 0.0f;
    delay += delta_time;

    if (delay > 1.0f) {
        delay = 0.0f;

        // 随机显示不同类型的消息
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

/**
 * @brief 运行场景2：文件操作
 */
void run_scene_file_operations(float delta_time) {
    static float delay = 0.0f;
    static int step = 0;
    delay += delta_time;

    if (delay > 2.0f) {
        delay = 0.0f;
        step++;

        switch (step) {
            case 1:
                ToastManager::instance().info("保存中", "正在保存文件...");
                break;
            case 2:
                ToastManager::instance().success("保存成功", "文件已成功保存到磁盘");
                break;
            case 3:
                ToastManager::instance().info("加载中", "正在加载文件...");
                break;
            case 4:
                ToastManager::instance().error("加载失败", "文件不存在或已损坏");
                step = 0;
                break;
        }
    }
}

/**
 * @brief 运行场景3：表单验证
 */
void run_scene_form_validation(float delta_time) {
    static float delay = 0.0f;
    static int step = 0;
    delay += delta_time;

    if (delay > 1.5f) {
        delay = 0.0f;
        step++;

        switch (step) {
            case 1:
                ToastManager::instance().warning("验证失败", "用户名不能为空");
                break;
            case 2:
                ToastManager::instance().warning("验证失败", "邮箱格式不正确");
                break;
            case 3:
                ToastManager::instance().success("注册成功", "您的账户已创建");
                step = 0;
                break;
        }
    }
}

/**
 * @brief 运行场景4：进度反馈
 */
void run_scene_progress(float delta_time) {
    static float delay = 0.0f;
    static int step = 0;
    delay += delta_time;

    if (delay > 1.0f) {
        delay = 0.0f;
        step++;

        if (step <= 5) {
            ToastManager::instance().info(
                "处理中",
                "正在处理第 " + std::to_string(step) + "/5 项..."
            );
        } else {
            ToastManager::instance().success("全部完成", "所有项目已处理完毕");
            step = 0;
        }
    }
}

/**
 * @brief 主循环
 */
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
                run_scene_form_validation(delta_time);
                break;
            case 4:
                run_scene_progress(delta_time);
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
    std::cout << "=============================" << std::endl;
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
