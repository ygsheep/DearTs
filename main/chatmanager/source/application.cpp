/**
 * @file application.cpp
 * @brief ChatManager 应用程序实现（SDL3 GPU 后端）
 */

#include "chatmanager/application.hpp"
#include "core/plugin/plugin.h"
#include "core/event/event_bus.h"
#include "core/content/commands.h"
#include "core/ui/view.h"
#include "core/ui/theme_manager.h"
#include "core/config/config_manager.h"
#include "plugins/memory_core/include/memory_core/memory_core_plugin.hpp"
#include "chat/chat_plugin.hpp"
#include "chat/ui/markdown_renderer.hpp"
#include "plugins/builtin/include/builtin_plugin.hpp"
#include "liblogger/logger.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui.h>
#include <imgui_internal.h>

// 使用 Logger 命名空间，以便 LOG_INFO 宏可以正常工作
using DearTs::Logger;

// 简化 Core 命名空间访问
namespace Core = DearTs::Core;

namespace ChatManager {

Application::Application() = default;
Application::~Application() = default;

bool Application::initialize() {
    LOG_INFO("ChatManager Application initializing...");

    // 在 SDL_Init 之前设置应用程序元数据（SDL3 推荐）
    SDL_SetAppMetadata("ChatManager", "1.0.0", "DearTs Team");

    // 检查 SDL_GetVersion 以确认 SDL3 被正确链接
    // SDL3 中 SDL_GetVersion() 返回 int，需要使用宏解析
    LOG_INFO("SDL3 compiled version: {}.{}.{}", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);

    // 获取运行时版本（需要链接 SDL 后才能调用）
    int version = SDL_GetVersion();
    LOG_INFO("SDL3 linked version: {}.{}.{}",
        SDL_VERSIONNUM_MAJOR(version),
        SDL_VERSIONNUM_MINOR(version),
        SDL_VERSIONNUM_MICRO(version));

    // 获取当前平台
    LOG_INFO("Current platform: {}", SDL_GetPlatform());

    // 1. 初始化 SDL3
    LOG_INFO("Attempting to initialize SDL3...");
    bool init_result = SDL_Init(SDL_INIT_VIDEO);

    if (!init_result) {
        const char* error = SDL_GetError();
        LOG_ERROR("SDL_Init failed: '{}'", error ? error : "(null)");

        // 尝试只初始化事件系统
        LOG_INFO("Trying SDL_InitSubSystem(SDL_INIT_EVENTS) to init only events...");
        if (!SDL_InitSubSystem(SDL_INIT_EVENTS)) {
            LOG_ERROR("SDL_InitSubSystem(EVENTS) also failed: {}", SDL_GetError());
        } else {
            LOG_INFO("SDL_InitSubSystem(EVENTS) succeeded - video subsystem is the issue");
            SDL_Quit();
        }
        return false;
    }
    LOG_INFO("SDL3 initialized successfully");

    // 2. 创建窗口
    if (!initialize_window()) {
        LOG_ERROR("Failed to initialize window");
        cleanup();
        return false;
    }

    // 3. 创建 GPU 设备（仅 Windows 支持的格式）
    Uint32 shader_formats = SDL_GPU_SHADERFORMAT_SPIRV |  // Vulkan
                            SDL_GPU_SHADERFORMAT_DXIL |    // DirectX 12
                            SDL_GPU_SHADERFORMAT_DXBC;     // DirectX 11
                            // METAL 仅在 macOS/iOS 上可用，这里移除

    SDL_GPUDevice* gpu_device = SDL_CreateGPUDevice(
        shader_formats,
        true,  // debug mode
        nullptr  // NULL = 让 SDL 自动选择最佳 GPU 驱动
    );

    if (!gpu_device) {
        LOG_ERROR("SDL_CreateGPUDevice failed: {}", SDL_GetError());
        cleanup();
        return false;
    }
    m_gpu_device = gpu_device;
    LOG_INFO("GPU device created");

    // 4. 声明 GPU 交换链
    SDL_Window* window = static_cast<SDL_Window*>(m_window);
    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window)) {
        LOG_ERROR("SDL_ClaimWindowForGPUDevice failed: {}", SDL_GetError());
        cleanup();
        return false;
    }

    // 设置交换链参数（VSYNC, SDR 等）
    if (!SDL_SetGPUSwapchainParameters(
        gpu_device,
        window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        SDL_GPU_PRESENTMODE_VSYNC
    )) {
        LOG_ERROR("SDL_SetGPUSwapchainParameters failed: {}", SDL_GetError());
        cleanup();
        return false;
    }
    LOG_INFO("GPU swapchain parameters set");

    // 5. 初始化 ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // ==================== 使用 ThemeManager 设置主题 ====================
    LOG_INFO("Applying ChatManager theme...");

    auto& theme_manager = Core::UI::ThemeManager::instance();

    // 设置暗色主题
    theme_manager.setTheme(Core::UI::Theme::Dark);

    // 应用 ImGui 样式
    theme_manager.applyImGuiStyle();

    // 应用玻璃态效果（现代化外观）
    theme_manager.setGlassAlpha(0.85f);
    theme_manager.setBorderRadius(12.0f);
    theme_manager.setAccentColor(ImVec4(0.855f, 0.467f, 0.337f, 1.0f));  // Claude Primary (#DA7656)
    theme_manager.applyGlassmorphismStyle();

    // 注册主题变更回调（支持运行时切换主题）
    theme_manager.onThemeChanged([](Core::UI::Theme new_theme) {
        LOG_INFO("Theme changed to: {}", Core::UI::ThemeManager::getThemeName(new_theme));
        // 主题变更后，ChatManager 组件会自动从 ThemeManager 获取新颜色
    });

    LOG_INFO("ChatManager theme applied successfully");

    // 加载字体
    LOG_INFO("开始加载字体...");
    float font_size = 16.0f;

    // 字体配置
    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    font_config.PixelSnapH = true;

    // 尝试加载中文字体
    LOG_INFO("尝试加载中文字体...");
    const char* font_paths[] = {
        "resources/fonts/OPPOSans-M.ttf",
        "resources/fonts/NotoSansSC-Regular.ttf",
        "../resources/fonts/OPPOSans-M.ttf",
        "../resources/fonts/NotoSansSC-Regular.ttf",
        "../../resources/fonts/OPPOSans-M.ttf",
        "../../resources/fonts/NotoSansSC-Regular.ttf"
    };

    bool font_loaded = false;
    for (const char* font_path : font_paths) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(
            font_path,
            font_size,
            &font_config,
            io.Fonts->GetGlyphRangesChineseFull()
        );

        if (font != nullptr) {
            io.FontDefault = font;
            LOG_INFO("成功加载中文字体: {} (大小: {:.1f}px)", font_path, font_size);
            font_loaded = true;

            // 合并图标字体到主字体
            static const ImWchar MaterialSymbolsRanges[] = {
                0xE000, 0xE8FF, // Material Symbols
                0,
            };

            ImFontConfig icon_config;
            icon_config.MergeMode = true;
            icon_config.PixelSnapH = true;
            icon_config.OversampleH = 2;
            icon_config.OversampleV = 2;

            const char* icon_font_paths[] = {
                "resources/fonts/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf",
                "../../resources/fonts/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf",
            };

            for (const char* icon_path : icon_font_paths) {
                if (io.Fonts->AddFontFromFileTTF(icon_path, font_size, &icon_config, MaterialSymbolsRanges) != nullptr) {
                    LOG_INFO("成功合并图标字体到主字体");
                    break;
                }
            }
            break;
        }
    }

    if (!font_loaded) {
        LOG_WARN("未能加载 resources/fonts 中的中文字体，尝试使用系统字体...");

        // Windows 平台系统字体回退
        const char* system_font_paths[] = {
            "C:/Windows/Fonts/msyh.ttc",      // Microsoft YaHei (微软雅黑)
            "C:/Windows/Fonts/msyhbd.ttc",    // Microsoft YaHei Bold
            "C:/Windows/Fonts/simhei.ttf",    // SimHei (黑体)
            "C:/Windows/Fonts/simsun.ttc",    // SimSun (宋体)
            "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",  // Linux WenQuanYi Zen Hei
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",  // Linux Liberation Sans
        };

        for (const char* font_path : system_font_paths) {
            ImFont* font = io.Fonts->AddFontFromFileTTF(
                font_path,
                font_size,
                &font_config,
                io.Fonts->GetGlyphRangesChineseFull()
            );

            if (font != nullptr) {
                io.FontDefault = font;
                LOG_INFO("成功加载系统字体: {} (大小: {:.1f}px)", font_path, font_size);
                font_loaded = true;
                break;
            }
        }

        // 如果系统字体也加载失败，使用默认字体
        if (!font_loaded) {
            LOG_WARN("未能加载系统字体，使用 ImGui 默认字体（可能无法正确显示中文）");
            io.Fonts->AddFontDefault();
        }
    }

    // 加载独立图标字体（用于纯图标按钮）
    LOG_INFO("加载独立图标字体...");
    if (DearTs::Core::UI::IconFont::loadMaterialSymbols(18.0f)) {
        LOG_INFO("独立图标字体加载成功");
    } else {
        LOG_WARN("独立图标字体加载失败");
    }

    // 配置 MarkdownRenderer 字体（标题字体和代码字体）
    LOG_INFO("配置 MarkdownRenderer 字体...");
    ImFont* h1_font = nullptr;
    ImFont* h2_font = nullptr;
    ImFont* h3_font = nullptr;
    ImFont* code_font = nullptr;

    // 尝试加载标题字体（H1, H2, H3）
    ImFontConfig heading_config;
    heading_config.OversampleH = 2;
    heading_config.OversampleV = 2;

    for (const char* font_path : font_paths) {
        if (!h1_font) {
            h1_font = io.Fonts->AddFontFromFileTTF(font_path, 24.0f, &heading_config, io.Fonts->GetGlyphRangesChineseFull());
        }
        if (!h2_font) {
            h2_font = io.Fonts->AddFontFromFileTTF(font_path, 20.0f, &heading_config, io.Fonts->GetGlyphRangesChineseFull());
        }
        if (!h3_font) {
            h3_font = io.Fonts->AddFontFromFileTTF(font_path, 16.0f, &heading_config, io.Fonts->GetGlyphRangesChineseFull());
        }
        if (h1_font && h2_font && h3_font) {
            LOG_INFO("MarkdownRenderer 标题字体加载成功");
            break;
        }
    }

    // 代码字体（使用等宽字体，如果找不到则回退到主字体）
    const char* code_font_paths[] = {
        "C:/Windows/Fonts/consola.ttf",   // Windows Consolas
        "C:/Windows/Fonts/consolaz.ttf",  // Windows Consolas Bold
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",  // Linux
    };

    for (const char* font_path : code_font_paths) {
        code_font = io.Fonts->AddFontFromFileTTF(font_path, 14.0f, &heading_config, io.Fonts->GetGlyphRangesChineseFull());
        if (code_font) {
            LOG_INFO("MarkdownRenderer 代码字体加载成功: {}", font_path);
            break;
        }
    }

    // 如果代码字体加载失败，使用主字体
    if (!code_font) {
        code_font = io.FontDefault;
        LOG_WARN("未能加载代码字体，使用主字体作为代码字体");
    }

    // 配置 MarkdownRenderer
    DearTs::Plugins::Chat::UI::MarkdownRenderer::set_fonts(h1_font, h2_font, h3_font, code_font);
    LOG_INFO("MarkdownRenderer 字体配置完成");

    // 6. 初始化 ImGui SDL3/GPU 后端
    if (!ImGui_ImplSDL3_InitForSDLGPU(window)) {
        LOG_ERROR("ImGui_ImplSDL3_InitForSDLGPU failed");
        cleanup();
        return false;
    }

    // 查询交换链纹理格式
    SDL_GPUTextureFormat swapchain_format = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);

    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = gpu_device;
    init_info.ColorTargetFormat = swapchain_format;
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;

    if (!ImGui_ImplSDLGPU3_Init(&init_info)) {
        LOG_ERROR("ImGui_ImplSDLGPU3_Init failed");
        cleanup();
        return false;
    }
    m_imgui_context = ImGui::GetCurrentContext();
    LOG_INFO("ImGui initialized with SDL3 GPU backend");

    // 7. 初始化 ConfigManager
    LOG_INFO("Initializing ConfigManager...");
    auto& config_manager = Core::Config::ConfigManager::instance();

    // 配置文件路径（与 exe 同目录）
    const char* config_file = "chat_config.json";

    // 尝试加载配置文件（如果存在）
    auto load_result = config_manager.load_from_file(config_file);
    if (load_result.isOk()) {
        LOG_INFO("Configuration loaded from: {}", config_file);
    } else {
        LOG_INFO("No existing configuration file found, will use defaults: {}", config_file);
    }

    // 设置全局变更回调（实时保存配置，带防抖）
    config_manager.add_change_callback([config_file,
                                        &config_manager](const std::string& key, const Core::Config::ConfigValue& old_val, const Core::Config::ConfigValue& new_val) {
        static auto last_save = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_save).count();

        // 防抖：500ms 内只保存一次
        if (elapsed >= 500) {
            last_save = now;
            LOG_DEBUG("Configuration changed: {}, saving to {}", key, config_file);
            config_manager.save_to_file(config_file);
        }
    });

    LOG_INFO("ConfigManager initialized successfully");

    // 8. 初始化插件系统
    if (!initialize_plugin_system()) {
        LOG_ERROR("Failed to initialize plugin system");
        cleanup();
        return false;
    }

    // 9. 设置自定义标题栏
    m_title_bar.set_borderless(true);
    DearTs::Core::UI::WindowControls::set_current_window(window);
    LOG_INFO("Custom title bar enabled");

    // 10. 显示窗口
    SDL_ShowWindow(window);

    m_is_initialized = true;
    m_is_running = true;

    LOG_INFO("ChatManager initialized successfully");
    return true;
}

bool Application::initialize_window() {
    // 设置无边框窗口
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_BORDERLESS
    );

    m_window = SDL_CreateWindow(
        "ChatManager",  // 无边框模式下标题由自定义标题栏绘制
        1600, 900,
        window_flags
    );

    if (!m_window) {
        LOG_ERROR("SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
    }

    LOG_INFO("Borderless window created: 1600x900");
    return true;
}

bool Application::initialize_plugin_system() {
    LOG_INFO("Initializing plugin system...");

    auto& plugin_manager = DearTs::Core::Plugin::PluginManager::instance();

    // 加载 builtin 插件（提供基础 UI 组件）
    auto result1 = plugin_manager.add_builtin(std::make_unique<DearTs::Plugins::Builtin::BuiltinPlugin>());
    if (!result1.isOk()) {
        LOG_ERROR("Failed to add builtin plugin: {}", result1.error());
        return false;
    }

    // 加载 memory_core 插件（持久化和 RAG 服务）
    auto result2 = plugin_manager.add_builtin(std::make_unique<DearTs::Plugins::MemoryCore::MemoryCorePlugin>());
    if (!result2.isOk()) {
        LOG_ERROR("Failed to add memory_core plugin: {}", result2.error());
        return false;
    }

    // 加载 chat 插件
    auto result3 = plugin_manager.add_builtin(std::make_unique<DearTs::Plugins::Chat::ChatPlugin>());
    if (!result3.isOk()) {
        LOG_ERROR("Failed to add chat plugin: {}", result3.error());
        return false;
    }

    // 手动启用每个插件
    auto enable_result1 = plugin_manager.enable("Builtin");
    if (!enable_result1.isOk()) {
        LOG_WARN("Builtin plugin enable: {}", enable_result1.error());
    }

    auto enable_result2 = plugin_manager.enable("memory_core");
    if (!enable_result2.isOk()) {
        LOG_WARN("memory_core plugin enable: {}", enable_result2.error());
    }

    auto enable_result3 = plugin_manager.enable("Chat");
    if (!enable_result3.isOk()) {
        LOG_WARN("Chat plugin enable: {}", enable_result3.error());
    }

    // 获取插件数量
    auto plugins_info = plugin_manager.get_all_plugins_info();
    LOG_INFO("Plugin system initialized: {} plugins loaded", plugins_info.size());
    return true;
}

void Application::run() {
    LOG_INFO("ChatManager main loop started");

    int frame_count = 0;
    while (m_is_running) {
        main_loop();
        frame_count++;
    }

    LOG_INFO("ChatManager main loop ended after {} frames", frame_count);
}

void Application::main_loop() {
    // 处理 SDL 事件
    process_events();

    // 处理异步事件
    DearTs::Core::Event::EventBus::instance().process_async_events();

    // 开始 ImGui 帧
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // 渲染帧
    render_frame();

    // 渲染 ImGui
    ImGui::Render();

    // GPU 渲染命令
    SDL_GPUDevice* gpu_device = static_cast<SDL_GPUDevice*>(m_gpu_device);
    SDL_Window* window = static_cast<SDL_Window*>(m_window);
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu_device);

    if (cmd) {
        // 获取交换链纹理
        SDL_GPUTexture* swapchain_texture = nullptr;
        if (SDL_AcquireGPUSwapchainTexture(cmd, window, &swapchain_texture, nullptr, nullptr)) {
            if (swapchain_texture != nullptr) {
                // 准备 ImGui 绘制数据（必须在 render_pass 之前调用！）
                ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), cmd);

                // 开始渲染通道
                SDL_GPUColorTargetInfo color_target = {};
                color_target.texture = swapchain_texture;
                color_target.mip_level = 0;
                color_target.layer_or_depth_plane = 0;
                color_target.clear_color.r = 0.1f;
                color_target.clear_color.g = 0.1f;
                color_target.clear_color.b = 0.1f;
                color_target.clear_color.a = 1.0f;
                color_target.load_op = SDL_GPU_LOADOP_CLEAR;
                color_target.store_op = SDL_GPU_STOREOP_STORE;

                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
                    cmd,
                    &color_target,
                    1,
                    nullptr
                );

                if (render_pass) {
                    // 渲染 ImGui 绘制数据（在 render_pass 之内调用）
                    ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), cmd, render_pass);
                    SDL_EndGPURenderPass(render_pass);
                }
            }
            // 如果 swapchain_texture 是 null，跳过渲染（窗口可能还没准备好）
        }

        SDL_SubmitGPUCommandBuffer(cmd);
    }

    // 限制帧率（VSYNC 由交换链参数控制）
    SDL_Delay(1);
}

void Application::process_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type) {
            case SDL_EVENT_QUIT:
                LOG_INFO("SDL_EVENT_QUIT received, exiting...");
                m_is_running = false;
                break;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                SDL_Window* window = static_cast<SDL_Window*>(m_window);
                if (event.window.windowID == SDL_GetWindowID(window)) {
                    m_is_running = false;
                }
                break;
            }

            // 窗口拖拽处理
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (event.button.y < 40) {  // 标题栏高度区域
                        // 检查是否在按钮区域外（避免拖拽按钮时触发窗口拖拽）
                        ImGuiViewport* viewport = ImGui::GetMainViewport();
                        if (event.button.x < viewport->Size.x - 100) {  // 右侧 100px 为按钮区域
                            m_is_dragging = true;
                            float global_mouse_x, global_mouse_y;
                            SDL_GetGlobalMouseState(&global_mouse_x, &global_mouse_y);
                            int window_x, window_y;
                            SDL_GetWindowPosition(static_cast<SDL_Window*>(m_window), &window_x, &window_y);
                            m_drag_start_pos = ImVec2(global_mouse_x, global_mouse_y);
                            m_window_start_pos = ImVec2(static_cast<float>(window_x), static_cast<float>(window_y));
                        }
                    }
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_is_dragging = false;
                }
                break;

            case SDL_EVENT_MOUSE_MOTION:
                if (m_is_dragging) {
                    float global_mouse_x, global_mouse_y;
                    SDL_GetGlobalMouseState(&global_mouse_x, &global_mouse_y);
                    float delta_x = global_mouse_x - m_drag_start_pos.x;
                    float delta_y = global_mouse_y - m_drag_start_pos.y;
                    SDL_SetWindowPosition(static_cast<SDL_Window*>(m_window),
                        static_cast<int>(m_window_start_pos.x + delta_x),
                        static_cast<int>(m_window_start_pos.y + delta_y));
                }
                break;

            case SDL_EVENT_KEY_DOWN:
                // 快捷键处理
                if (event.key.key == SDLK_ESCAPE && (event.key.mod & SDL_KMOD_ALT)) {
                    m_is_running = false;
                }
                // Ctrl+N: 新建会话
                if (event.key.key == SDLK_N && (event.key.mod & SDL_KMOD_CTRL)) {
                    DearTs::Core::ContentRegistry::Commands::Registry::instance().execute("chat.new_conversation");
                }
                break;

            default:
                break;
        }
    }
}

void Application::render_frame() {
    // 1. 渲染自定义标题栏（返回标题栏高度）
    float title_bar_height = render_title_bar();

    // 2. 创建 DockSpace（考虑标题栏高度偏移）
    render_dock_space(title_bar_height);

    // 3. 渲染所有视图
    // 视图会自动通过 ContentRegistry::Views 渲染
    for (auto& [name, view] : Core::ContentRegistry::Views::get_all()) {
        view->track_window_state();
        if (view->should_draw()) {
            view->draw();
        }
    }
}

void Application::cleanup() {
    LOG_INFO("Cleaning up ChatManager resources...");

    // 清理 ImGui
    if (m_imgui_context) {
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        m_imgui_context = nullptr;
        LOG_INFO("ImGui shutdown");
    }

    // 清理 GPU 设备
    if (m_gpu_device && m_window) {
        SDL_GPUDevice* gpu_device = static_cast<SDL_GPUDevice*>(m_gpu_device);
        SDL_Window* window = static_cast<SDL_Window*>(m_window);
        SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
    }

    if (m_gpu_device) {
        SDL_DestroyGPUDevice(static_cast<SDL_GPUDevice*>(m_gpu_device));
        m_gpu_device = nullptr;
        LOG_INFO("GPU device destroyed");
    }

    // 清理窗口
    if (m_window) {
        SDL_DestroyWindow(static_cast<SDL_Window*>(m_window));
        m_window = nullptr;
        LOG_INFO("Window destroyed");
    }

    // 清理 SDL
    SDL_Quit();
    LOG_INFO("SDL3 shutdown");
}

float Application::render_title_bar() {
    float title_bar_height = 0.0f;

    if (!m_title_bar.is_borderless()) {
        return title_bar_height;
    }

    // 设置标题栏样式
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 10));

    if (ImGui::BeginMainMenuBar()) {
        title_bar_height = ImGui::GetFrameHeight();

        // 左侧：应用标题
        ImGui::SetCursorPosX(10);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));

        // 使用图标字体渲染图标（如果已加载）
        if (Core::UI::IconFont::isLoaded()) {
            ImGui::PushFont(Core::UI::IconFont::getFont());
            ImGui::Text("%s", ICON_CHAT);
            ImGui::PopFont();
            ImGui::SameLine(0, 0);
            ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f), "ChatManager");
        } else {
            // 图标字体未加载，使用纯文本
            ImGui::Text("ChatManager");
        }
        ImGui::PopStyleColor();

        // 右侧：窗口控制按钮
        const float button_size = 30.0f;
        const float spacing = 8.0f;
        float button_y = (title_bar_height - button_size) / 2.0f;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float start_x = viewport->Size.x - (3 * button_size + 2 * spacing + 10);

        // 使用图标字体（如果已加载）
        if (Core::UI::IconFont::isLoaded()) {
            ImGui::PushFont(Core::UI::IconFont::getFont());
        }

        // 最小化按钮
        ImGui::SetCursorPos(ImVec2(start_x, button_y));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        if (ImGui::Button(ICON_MINIMIZE, ImVec2(button_size, button_size))) {
            SDL_MinimizeWindow(static_cast<SDL_Window*>(m_window));
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        // 最大化按钮
        ImGui::SetCursorPos(ImVec2(start_x + button_size + spacing, button_y));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        if (ImGui::Button(ICON_MAXIMIZE, ImVec2(button_size, button_size))) {
            SDL_MaximizeWindow(static_cast<SDL_Window*>(m_window));
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        // 关闭按钮
        ImGui::SetCursorPos(ImVec2(start_x + (button_size + spacing) * 2, button_y));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        if (ImGui::Button(ICON_CLOSE, ImVec2(button_size, button_size))) {
            m_is_running = false;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        // 恢复原来的字体
        if (Core::UI::IconFont::isLoaded()) {
            ImGui::PopFont();
        }

        ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleVar();

    return title_bar_height;
}

void Application::render_dock_space(float title_bar_height) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float menu_bar_height = m_title_bar.is_borderless() ? title_bar_height : 0.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menu_bar_height));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - menu_bar_height));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                   ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBackground |
                                   ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // 设置默认停靠布局（仅在第一次运行时）
    setup_default_dock_layout();

    ImGui::DockSpace(ImGui::GetID("MainDockSpace"));
    ImGui::End();
}

void Application::setup_default_dock_layout() {
    // 检查是否已经设置过停靠布局
    if (m_dock_layout_initialized) {
        return;
    }

    // 获取主 DockSpace ID
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

    // 使用 DockBuilder 构建默认停靠布局
    ImGui::DockBuilderRemoveNode(dockspace_id);  // 清除现有布局
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);  // 根节点

    ImGuiID dock_main_id = dockspace_id;
    ImGuiID dock_left_id;
    ImGuiID dock_center_id;

    // 分割左侧（会话列表）和中间区域
    // 比例：左侧约 19%
    dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.19f, nullptr, &dock_center_id);

    // 分割右侧（信息面板）和中间区域
    // 比例：右侧约 23%
    ImGuiID dock_right_id;
    dock_right_id = ImGui::DockBuilderSplitNode(dock_center_id, ImGuiDir_Right, 0.23f, nullptr, &dock_center_id);

    // 分割中间区域：上面是聊天，下面是输入
    // 比例：聊天约 66%
    ImGuiID dock_chat_id;
    ImGuiID dock_input_id;
    dock_chat_id = ImGui::DockBuilderSplitNode(dock_center_id, ImGuiDir_Down, 0.66f, nullptr, &dock_input_id);

    // 停靠窗口到对应的节点
    ImGui::DockBuilderDockWindow("会话", dock_left_id);
    ImGui::DockBuilderDockWindow("聊天", dock_chat_id);
    ImGui::DockBuilderDockWindow("输入", dock_input_id);
    ImGui::DockBuilderDockWindow("信息", dock_right_id);

    // 完成构建
    ImGui::DockBuilderFinish(dockspace_id);

    m_dock_layout_initialized = true;
    LOG_INFO("Default dock layout configured");
}

void Application::shutdown() {
    LOG_INFO("Shutting down ChatManager...");

    cleanup();

    LOG_INFO("ChatManager shutdown complete");
}

} // namespace ChatManager
