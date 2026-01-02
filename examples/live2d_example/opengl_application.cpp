/**
 * @file opengl_application.cpp
 * @brief OpenGL 应用程序实现
 */

#include "opengl_application.hpp"
#include "../../plugins/live2d/include/live2d_plugin.hpp"
#include <iostream>
#include <format>

namespace Live2DExample {

// 全局 Live2D 插件实例
static DearTs::Plugins::Live2D::Live2DPlugin* g_live2d_plugin = nullptr;

OpenGLApplication::OpenGLApplication()
    : m_window(nullptr)
    , m_gl_context(nullptr)
    , m_running(false)
    , m_delta_time(0.0)
{
}

OpenGLApplication::~OpenGLApplication() {
    shutdown();
}

bool OpenGLApplication::initialize() {
    // 1. 初始化 SDL
    if (!init_sdl()) {
        return false;
    }

    // 2. 创建窗口
    if (!create_window()) {
        return false;
    }

    // 3. 初始化 OpenGL
    if (!init_opengl()) {
        return false;
    }

    // 4. 初始化 ImGui
    if (!init_imgui()) {
        return false;
    }

    // 5. 创建并初始化 Live2D 插件
    create_live2d_ui();

    m_running = true;
    m_last_frame_time = std::chrono::steady_clock::now();

    std::cout << "OpenGL Application initialized successfully\n";
    return true;
}

bool OpenGLApplication::init_sdl() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << std::format("Failed to initialize SDL: {}\n", SDL_GetError());
        return false;
    }

    // 设置 OpenGL 属性（使用兼容模式以支持 Live2D）
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);  // OpenGL 3.0 兼容模式
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    std::cout << "SDL initialized\n";
    return true;
}

bool OpenGLApplication::create_window() {
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    m_window = SDL_CreateWindow(
        WINDOW_TITLE,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        window_flags
    );

    if (!m_window) {
        std::cerr << std::format("Failed to create window: {}\n", SDL_GetError());
        return false;
    }

    std::cout << "Window created\n";
    return true;
}

bool OpenGLApplication::init_opengl() {
    // 创建 OpenGL 上下文
    m_gl_context = SDL_GL_CreateContext(m_window);
    if (!m_gl_context) {
        std::cerr << std::format("Failed to create OpenGL context: {}\n", SDL_GetError());
        return false;
    }

    // 设置 VSync
    if (!SDL_GL_SetSwapInterval(1)) {
        std::cerr << std::format("Warning: Failed to set VSync: {}\n", SDL_GetError());
    }

    // 初始化 GLEW（OpenGL 加载器）
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << std::format("Failed to initialize GLEW: {}\n",
            reinterpret_cast<const char*>(glewGetErrorString(err)));
        return false;
    }

    // 获取 OpenGL 版本
    std::cout << std::format("OpenGL {} initialized (GLEW {})\n",
        reinterpret_cast<const char*>(glGetString(GL_VERSION)),
        reinterpret_cast<const char*>(glewGetString(GLEW_VERSION)));

    return true;
}

bool OpenGLApplication::init_imgui() {
    // 创建 ImGui 上下文
    IMGUI_CHECKVERSION();
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
    // 设置样式
    ImGui::StyleColorsDark();

    // 初始化 ImGui SDL3 + OpenGL3 后端
    if (!ImGui_ImplSDL3_InitForOpenGL(m_window, m_gl_context)) {
        std::cerr << "Failed to initialize ImGui SDL3 backend\n";
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init()) {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend\n";
        return false;
    }

    std::cout << "ImGui initialized\n";
    return true;
}

void OpenGLApplication::create_live2d_ui() {
    std::cout << "Initializing Live2D Plugin...\n";

    // 创建 Live2D 插件实例
    g_live2d_plugin = new DearTs::Plugins::Live2D::Live2DPlugin();

    // 加载插件
    auto result = g_live2d_plugin->on_load();
    if (result.isErr()) {
        std::cerr << std::format("Failed to load Live2D plugin: {}\n", result.error());
        delete g_live2d_plugin;
        g_live2d_plugin = nullptr;
        return;
    }

    // 启用插件
    g_live2d_plugin->on_enable();

    std::cout << "Live2D Plugin loaded successfully\n";
}

int OpenGLApplication::run() {
    while (m_running) {
        process_events();
        update();
        render();
    }

    return 0;
}

void OpenGLApplication::process_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // 处理 ImGui 事件
        ImGui_ImplSDL3_ProcessEvent(&event);

        // 处理窗口事件
        if (event.type == SDL_EVENT_QUIT) {
            m_running = false;
        }
        else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            if (SDL_Window* window = SDL_GetWindowFromEvent(&event)) {
                if (window == m_window) {
                    m_running = false;
                }
            }
        }
    }
}

void OpenGLApplication::update() {
    // 计算时间增量
    auto current_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = current_time - m_last_frame_time;
    m_delta_time = elapsed.count();
    m_last_frame_time = current_time;

    // 更新 Live2D 插件
    if (g_live2d_plugin) {
        g_live2d_plugin->update(static_cast<float>(m_delta_time));
    }
}

void OpenGLApplication::render() {
    // 1. 清空屏幕
    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. ImGui NewFrame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // 3. 创建 DockSpace
    ImGui::DockSpaceOverViewport();

    // 4. 渲染主窗口
    if (ImGui::Begin("Live2D Demo")) {
        ImGui::Text("Live2D 插件示例程序");
        ImGui::Text("FPS: %.1f", 1.0 / m_delta_time);
        ImGui::Separator();

        // 显示 Live2D 插件信息
        if (g_live2d_plugin) {
            auto model_names = g_live2d_plugin->get_model_names();
            const auto* active_model = g_live2d_plugin->get_active_model();

            ImGui::Text("已加载模型: %zu", model_names.size());
            ImGui::Text("活动模型: %s",
                active_model ? active_model->get_info().model_name.c_str() : "无");

            ImGui::Separator();

            // 模型选择
            if (ImGui::BeginCombo("选择模型", active_model ? active_model->get_info().model_name.c_str() : "未选择")) {
                for (const auto& name : model_names) {
                    const bool is_selected = (active_model && active_model->get_info().model_name == name);
                    if (ImGui::Selectable(name.c_str(), is_selected)) {
                        g_live2d_plugin->set_active_model(name);
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        } else {
            ImGui::Text("Live2D 插件未加载");
        }
    }
    ImGui::End();

    // 5. 渲染 Live2D 模型（在 ImGui 之前）
    if (g_live2d_plugin) {
        g_live2d_plugin->render();
    }

    // 6. 渲染 ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 7. 交换缓冲区
    SDL_GL_SwapWindow(m_window);
}

void OpenGLApplication::shutdown() {
    if (!m_window) {
        return;
    }

    // 清理 Live2D 插件
    if (g_live2d_plugin) {
        g_live2d_plugin->on_disable();
        g_live2d_plugin->on_unload();
        delete g_live2d_plugin;
        g_live2d_plugin = nullptr;
        std::cout << "Live2D Plugin cleaned up\n";
    }

    // 清理 ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    // 清理 OpenGL 上下文
    if (m_gl_context) {
        SDL_GL_DestroyContext(m_gl_context);
        m_gl_context = nullptr;
    }

    // 清理窗口
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    // 清理 SDL
    SDL_Quit();

    std::cout << "OpenGL Application shut down\n";
}

} // namespace Live2DExample
