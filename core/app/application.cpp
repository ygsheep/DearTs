/**
 * @file application.cpp
 * @brief 应用程序基类实现
 */

#include "application.h"
#include "../content/callbacks.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace DearTs {
namespace Core {
namespace App {

Application::Application()
    : m_state(ApplicationState::UNINITIALIZED)
    , m_exit_code(0)
    , m_should_exit(false)
    , m_delta_time(0.0)
    , m_frame_count(0)
    , m_fps_frame_count(0)
    , m_current_fps(0.0)
    , m_average_fps(0.0)
    , m_window(nullptr)
    , m_renderer(nullptr)
    , m_enable_vsync(true)
    , m_target_fps(60)
    , m_frame_time(1.0 / 60.0)
{
    LOG_INFO("Application created");
}

Application::~Application() {
    // 如果应用程序还在运行，先关闭
    if (m_state != ApplicationState::STOPPED &&
        m_state != ApplicationState::UNINITIALIZED) {
        shutdown();
    }
    LOG_INFO("Application destroyed");
}

bool Application::initialize(const ApplicationConfig& config) {
    if (m_state != ApplicationState::UNINITIALIZED) {
        LOG_ERROR("Application already initialized");
        return false;
    }

    m_state = ApplicationState::INITIALIZING;
    m_config = config;
    m_enable_vsync = config.enable_vsync;

    LOG_INFO("=== Initializing Application ===");
    LOG_INFO("Application: {} v{}", m_config.name, m_config.version);
    LOG_INFO("Window size: {}x{}", m_config.window_width, m_config.window_height);

    // 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        LOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
        m_state = ApplicationState::STOPPED;
        return false;
    }
    LOG_INFO("SDL3 initialized successfully");

    // 创建窗口
    Uint32 window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
    if (m_config.borderless) {
        window_flags |= SDL_WINDOW_BORDERLESS;
        LOG_INFO("Borderless window mode enabled");
    }

    m_window = SDL_CreateWindow(
        m_config.name.c_str(),
        m_config.window_width,
        m_config.window_height,
        window_flags
    );

    if (!m_window) {
        LOG_ERROR("Failed to create SDL window: {}", SDL_GetError());
        SDL_Quit();
        m_state = ApplicationState::STOPPED;
        return false;
    }
    LOG_INFO("SDL window created successfully");

    // 设置窗口图标
    // 注意：Windows 资源文件（dearts.rc）中的图标会自动显示在任务栏和文件管理器
    // 这里设置的图标主要用于窗口标题栏（如果有）和其他平台
#ifdef _WIN32
    // Windows: 从资源文件加载图标
    // 由于已经通过 dearts.rc 嵌入了图标，任务栏和文件管理器会自动显示
    // 这里只是尝试加载用于运行时的图标（borderless 窗口中效果不明显）
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));  // IDI_MAIN_ICON
    if (hIcon) {
        // SDL3 在 Windows 上会自动使用嵌入的图标
        LOG_INFO("Windows icon loaded from resources");
    }
#else
    // 其他平台: 尝试从文件加载图标（支持 PNG/BMP/ICO）
    const char* icon_paths[] = {
        "resources/icon.png",
        "resources/icon.bmp",
        "resources/icon.ico",
        "icon.png",
        "icon.bmp"
    };

    bool icon_loaded = false;
    for (const char* icon_path : icon_paths) {
        SDL_Surface* icon_surface = SDL_LoadBMP(icon_path);
        if (icon_surface) {
            SDL_SetWindowIcon(m_window, icon_surface);
            SDL_DestroySurface(icon_surface);
            LOG_INFO("Window icon loaded from: {}", icon_path);
            icon_loaded = true;
            break;
        }
    }

    if (!icon_loaded) {
        LOG_WARN("Failed to load window icon, using default");
    }
#endif

    // 创建渲染器
    // SDL3 使用不同的 API: SDL_CreateRenderer(window, name)
    // 或者使用 SDL_CreateRendererWithProperties 配置选项
    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
        LOG_ERROR("Failed to create SDL renderer: {}", SDL_GetError());
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        m_window = nullptr;
        m_state = ApplicationState::STOPPED;
        return false;
    }
    LOG_INFO("SDL renderer created successfully");

    // 设置 VSync (SDL3 新 API)
    if (m_enable_vsync) {
        if (SDL_SetRenderVSync(m_renderer, 1)) {
            LOG_INFO("VSync enabled");
        } else {
            LOG_WARN("Failed to enable VSync: {}", SDL_GetError());
        }
    }

    // 调用子类初始化回调
    LOG_INFO("Calling user on_init()...");
    if (!on_init()) {
        LOG_ERROR("User on_init() failed");
        shutdown();
        return false;
    }

    // 显示窗口
    SDL_ShowWindow(m_window);

    // 初始化时间
    m_last_frame_time = std::chrono::steady_clock::now();
    m_fps_timer = m_last_frame_time;
    m_frame_count = 0;
    m_fps_frame_count = 0;

    m_state = ApplicationState::RUNNING;
    LOG_INFO("=== Application initialized successfully ===");
    LOG_INFO("Application is now running");

    return true;
}

int Application::run() {
    if (m_state != ApplicationState::RUNNING) {
        LOG_ERROR("Application not in running state, cannot run");
        return -1;
    }

    LOG_INFO("=== Starting main loop ===");

    while (!m_should_exit.load() && m_state == ApplicationState::RUNNING) {
        // 计算时间增量
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = current_time - m_last_frame_time;
        m_delta_time = elapsed.count();
        m_last_frame_time = current_time;

        // 处理事件
        process_events();

        // 更新逻辑
        on_update(m_delta_time);

        // 调用注册的更新回调
        DearTs::Core::ContentRegistry::Callbacks::run_update_callbacks(m_delta_time);

        // 渲染
        on_render();

        // 调用注册的渲染回调
        DearTs::Core::ContentRegistry::Callbacks::run_render_callbacks();

        // 呈现
        SDL_RenderPresent(m_renderer);

        // 更新统计信息
        update_stats();

        // 限制帧率
        if (!m_enable_vsync) {
            limit_frame_rate();
        }

        // 帧计数
        m_frame_count++;
        m_fps_frame_count++;
    }

    LOG_INFO("=== Main loop ended ===");
    LOG_INFO("Total frames: {}", m_frame_count);
    LOG_INFO("Average FPS: {:.2f}", m_average_fps);

    return m_exit_code.load();
}

void Application::shutdown() {
    if (m_state == ApplicationState::STOPPED) {
        LOG_WARN("Application already shut down");
        return;
    }

    if (m_state == ApplicationState::UNINITIALIZED) {
        LOG_WARN("Application was never initialized");
        return;
    }

    m_state = ApplicationState::STOPPING;

    LOG_INFO("=== Shutting down application ===");

    // 调用用户关闭回调
    LOG_INFO("Calling user on_shutdown()...");
    on_shutdown();

    // 清理渲染器
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
        LOG_INFO("SDL renderer destroyed");
    }

    // 清理窗口
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        LOG_INFO("SDL window destroyed");
    }

    // 清理 SDL
    SDL_Quit();
    LOG_INFO("SDL3 shut down");

    m_state = ApplicationState::STOPPED;
    LOG_INFO("=== Application shutdown complete ===");
}

void Application::request_exit(int exit_code) {
    LOG_INFO("Exit requested with code: {}", exit_code);
    m_exit_code.store(exit_code);
    m_should_exit.store(true);
}

void Application::process_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // 处理退出事件
        if (event.type == SDL_EVENT_QUIT) {
            request_exit(0);
            continue;
        }

        // 处理窗口关闭事件
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            if (event.window.windowID == SDL_GetWindowID(m_window)) {
                request_exit(0);
                continue;
            }
        }

        // 调用用户事件处理
        on_event(event);
    }
}

void Application::limit_frame_rate() {
    if (m_target_fps == 0) {
        return; // 无限制
    }

    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = current_time - m_last_frame_time;
    auto target_duration = std::chrono::duration<double>(m_frame_time);

    if (elapsed < target_duration) {
        auto sleep_time = target_duration - elapsed;
        auto sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(sleep_time);
        if (sleep_ms.count() > 0) {
            SDL_Delay(static_cast<Uint32>(sleep_ms.count()));
        }
    }
}

void Application::update_stats() {
    auto current_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = current_time - m_fps_timer;

    // 每秒更新一次 FPS
    if (elapsed.count() >= 1.0) {
        m_current_fps = static_cast<double>(m_fps_frame_count) / elapsed.count();

        if (m_average_fps == 0.0) {
            m_average_fps = m_current_fps;
        } else {
            // 简单的移动平均
            m_average_fps = m_average_fps * 0.9 + m_current_fps * 0.1;
        }

        m_fps_frame_count = 0;
        m_fps_timer = current_time;

        // LOG_DEBUG("FPS: {:.2f} (Average: {:.2f})", m_current_fps, m_average_fps);
    }
}

} // namespace App
} // namespace Core
} // namespace DearTs
