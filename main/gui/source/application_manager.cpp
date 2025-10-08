#include "../include/application_manager.h"
#include "../../../lib/libdearts/include/dearts/api/dearts_api.hpp"
#include "../../../lib/libdearts/include/dearts/api/event_manager.hpp"
#include "../../../lib/libdearts/include/dearts/api/plugin_manager.hpp"
#include "../../../core/resource/resource_manager.h"
#include "resource/font_resource.h"
#include <iostream>
#include <filesystem>

namespace DearTs::GUI {

/**
 * ApplicationManager构造函数
 * 初始化所有子系统
 */
ApplicationManager::ApplicationManager() 
    : m_window(nullptr), m_renderer(nullptr), m_windowManager(nullptr),
      m_running(false), m_initialized(false) {
    
    // 简单的日志输出
    std::cout << "ApplicationManager constructed" << std::endl;
}

/**
 * ApplicationManager析构函数
 * 清理所有资源
 */
ApplicationManager::~ApplicationManager() {
    shutdown();
}

/**
 * 初始化应用程序
 * 按顺序初始化所有子系统
 */
bool ApplicationManager::initialize() {
    if (m_initialized) {
        return true;
    }
    
    try {
        // 1. 初始化SDL
        if (!initializeSDL()) {
            throw std::runtime_error("Failed to initialize SDL");
        }
        
        // 2. 初始化ResourceManager
        if (!initializeResourceManager()) {
            throw std::runtime_error("Failed to initialize ResourceManager");
        }
        
        // 3. 初始化ImGui
        if (!initializeImGui()) {
            throw std::runtime_error("Failed to initialize ImGui");
        }
        
        // 4. 初始化窗口管理器
        if (!initializeWindowManager()) {
            throw std::runtime_error("Failed to initialize WindowManager");
        }
        
        // 5. 初始化主题系统
        if (!initializeTheme()) {
            throw std::runtime_error("Failed to initialize Theme system");
        }
        
        // 6. 初始化字体系统
        if (!initializeFonts()) {
            throw std::runtime_error("Failed to initialize Font system");
        }
        
        // 7. 初始化插件系统
        if (!initializePlugins()) {
            throw std::runtime_error("Failed to initialize Plugin system");
        }
        
        // 8. 注册事件处理器
        registerEventHandlers();
        
        // 9. 发布应用程序初始化完成事件
        ApplicationInitializedEvent::post(this);
        
        m_initialized = true;
        m_running = true;
        
        std::cout << "ApplicationManager initialized successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "ApplicationManager initialization failed: " << e.what() << std::endl;
        return false;
    }
}

/**
 * 运行主循环
 * 处理事件、更新状态、渲染界面
 */
void ApplicationManager::run() {
    if (!m_initialized) {
        std::cerr << "ApplicationManager not initialized" << std::endl;
        return;
    }
    
    std::cout << "Starting main loop..." << std::endl;
    
    // 主循环
    while (m_running) {
        // 处理事件
        processEvents();

        // 更新状态
        update();

        // 渲染界面
        render();

        // 调试：检查运行状态
        static int loop_count = 0;
        loop_count++;
        if (loop_count % 100 == 0) {
            std::cout << "GUI主循环运行中，m_running: " << (m_running ? "true" : "false") << ", loop_count: " << loop_count << std::endl;
        }
    }
    
    std::cout << "Main loop ended" << std::endl;
}

/**
 * 关闭应用程序
 * 清理所有资源
 */
void ApplicationManager::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down ApplicationManager..." << std::endl;
    
    // 发布应用程序关闭事件
    ApplicationShutdownEvent::post(this);
    
    // 取消事件处理器注册
    unregisterEventHandlers();
    
    // 关闭插件系统
    shutdownPlugins();
    
    // 清理窗口管理器
    if (m_windowManager) {
        delete m_windowManager;
        m_windowManager = nullptr;
    }
    
    // 清理ResourceManager
    auto* resourceManager = DearTs::Core::Resource::ResourceManager::getInstance();
    resourceManager->shutdown();
    
    // 清理ImGui
    shutdownImGui();
    
    // 清理SDL
    shutdownSDL();
    
    m_initialized = false;
    m_running = false;
    
    std::cout << "ApplicationManager shutdown complete" << std::endl;
}

/**
 * 请求退出应用程序
 */
void ApplicationManager::requestExit() {
    std::cout << "GUI ApplicationManager::requestExit() called, setting m_running = false" << std::endl;
    m_running = false;

    // 发布退出请求事件
    ApplicationExitRequestedEvent::post(this);
}

// 私有方法实现

/**
 * 初始化SDL
 */
bool ApplicationManager::initializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // 创建窗口
    m_window = SDL_CreateWindow(
        "DearTs Application",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE
    );
    
    if (!m_window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // 创建渲染器
    m_renderer = SDL_CreateRenderer(
        m_window, -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
    );
    
    if (!m_renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    return true;
}

/**
 * 初始化ResourceManager
 */
bool ApplicationManager::initializeResourceManager() {
    auto* resourceManager = DearTs::Core::Resource::ResourceManager::getInstance();
    
    if (!resourceManager->initialize(m_renderer)) {
        std::cerr << "ResourceManager initialization failed" << std::endl;
        return false;
    }
    
    std::cout << "ResourceManager initialized successfully" << std::endl;
    return true;
}

/**
 * 初始化ImGui
 */
bool ApplicationManager::initializeImGui() {
    // 检查ImGui版本
    IMGUI_CHECKVERSION();
    
    // 创建ImGui上下文
    ImGui::CreateContext();
    
    // 配置ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef ImGuiConfigFlags_DockingEnable
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
#ifdef ImGuiConfigFlags_ViewportsEnable
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
    
    // 初始化ImGui SDL2绑定
    if (!ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer)) {
        std::cerr << "ImGui SDL2 initialization failed" << std::endl;
        return false;
    }
    
    // 初始化ImGui SDL2渲染器绑定
    if (!ImGui_ImplSDLRenderer2_Init(m_renderer)) {
        std::cerr << "ImGui SDL2 renderer initialization failed" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * 初始化窗口管理器
 */
bool ApplicationManager::initializeWindowManager() {
    m_windowManager = new WindowManager(m_window);
    
    if (!m_windowManager->initialize()) {
        std::cerr << "WindowManager initialization failed" << std::endl;
        delete m_windowManager;
        m_windowManager = nullptr;
        return false;
    }
    
    m_windowManager->setWindowTitle("DearTs Application - Modern Interface");
    return true;
}

/**
 * 初始化主题系统
 */
bool ApplicationManager::initializeTheme() {
    // 使用DearTs API设置默认主题
    dearts::DearTsApi::Theme::setCurrentTheme("Dark");
    
    // 应用ImHex风格的颜色方案
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    
    // 设置颜色
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    
    return true;
}

/**
 * 初始化字体系统
 */
bool ApplicationManager::initializeFonts() {
    // ImGui会自动加载默认字体，这里可以加载自定义字体
    // 如果需要加载自定义字体，可以使用 dearts::DearTsApi::Fonts::loadFont
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    if (fontManager && fontManager->initialize()) {
        // 加载默认字体（与demo/old保持一致）
        std::cout << "正在加载默认字体..." << std::endl;
        if (!fontManager->loadDefaultFont(14.0f)) {
            std::cerr << "❌ 默认字体加载失败" << std::endl;
            // 继续运行，使用ImGui默认字体
        }
        return true;
    } else {
        std::cerr << "❌ 字体管理器初始化失败" << std::endl;
        // 继续运行，使用ImGui默认字体
        return false;
    }

}

/**
 * 初始化插件系统
 */
bool ApplicationManager::initializePlugins() {
    // 加载内置插件
    std::filesystem::path pluginsDir = "plugins/builtin";
    
    if (std::filesystem::exists(pluginsDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(pluginsDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                try {
                    dearts::PluginManager::load(entry.path().string());
                    std::cout << "Loaded plugin: " << entry.path().filename() << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Failed to load plugin " << entry.path().filename() 
                              << ": " << e.what() << std::endl;
                }
            }
        }
    }
    
    return true;
}

/**
 * 处理事件
 */
void ApplicationManager::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // ImGui事件处理
        ImGui_ImplSDL2_ProcessEvent(&event);
        
        // 窗口管理器事件处理
        if (m_windowManager) {
            m_windowManager->handleEvent(event);
        }
        
        // 应用程序级别事件处理
        switch (event.type) {
            case SDL_QUIT:
                std::cout << "GUI ApplicationManager: SDL_QUIT event received, requesting exit" << std::endl;
                requestExit();
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE &&
                    event.window.windowID == SDL_GetWindowID(m_window)) {
                    requestExit();
                }
                break;
        }
    }


}

/**
 * 更新状态
 */
void ApplicationManager::update() {
    // 更新插件系统
    // TODO: PluginManager::update() method does not exist in the API
    // Consider implementing plugin update logic if needed
    
    // 发布更新事件
    dearts::EventManager::post<ApplicationUpdateEvent>(this);
}

/**
 * 渲染界面
 */
void ApplicationManager::render() {
    // 清屏
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
    
    // 开始新帧
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    // 渲染自定义标题栏
    if (m_windowManager) {
        m_windowManager->renderTitleBar();
    }
    
    // 渲染主界面
    renderMainInterface();
    
    // 渲染插件界面
    // TODO: PluginManager::renderAll() method does not exist in the API
    // Consider implementing plugin rendering logic if needed
    
    // 结束帧
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);
    
    // 处理多视口
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
#endif
    
    // 呈现
    SDL_RenderPresent(m_renderer);
}

/**
 * 渲染主界面
 */
void ApplicationManager::renderMainInterface() {
    // 创建主停靠空间
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + m_windowManager->getTitleBarHeight()));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - m_windowManager->getTitleBarHeight()));
#ifdef IMGUI_HAS_VIEWPORT
    ImGui::SetNextWindowViewport(viewport->ID);
#endif
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
#ifdef IMGUI_HAS_DOCK
    window_flags |= ImGuiWindowFlags_NoDocking;
#endif
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    if (ImGui::Begin("DockSpace", nullptr, window_flags)) {
        ImGui::PopStyleVar(3);
        
        // 创建停靠空间
#ifdef IMGUI_HAS_DOCK
        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
#endif
        
        // 渲染菜单栏
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("文件")) {
                if (ImGui::MenuItem("新建文件", "Ctrl+N")) {
                    // 处理新建文件
                }
                if (ImGui::MenuItem("打开文件", "Ctrl+O")) {
                    // 处理打开文件
                }
                if (ImGui::MenuItem("保存文件", "Ctrl+S")) {
                    // 处理保存文件
                }
                ImGui::Separator();
                if (ImGui::MenuItem("退出", "Alt+F4")) {
                    requestExit();
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("窗口")) {
                if (ImGui::MenuItem("Reset Layout")) {
                    // 重置布局
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("工具")) {
                if (ImGui::MenuItem("设置")) {
                    // 打开设置
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("帮助")) {
                if (ImGui::MenuItem("关于")) {
                    // 显示关于对话框
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
    } else {
        ImGui::PopStyleVar(3);
    }
    
    ImGui::End();
}

/**
 * 注册事件处理器
 */
void ApplicationManager::registerEventHandlers() {
    // 注册插件加载事件
    // TODO: Replace with proper event subscription using dearts::EventManager
    // dearts::EventManager::subscribe<PluginLoadedEvent>([this](const PluginLoadedEvent& event) {
    //     std::cout << "Plugin loaded event received" << std::endl;
    // });
    
    // 注册插件卸载事件
    // TODO: Replace with proper event subscription using dearts::EventManager
    // dearts::EventManager::subscribe<PluginUnloadedEvent>([this](const PluginUnloadedEvent& event) {
    //     std::cout << "Plugin unloaded event received" << std::endl;
    // });
}

/**
 * 取消事件处理器注册
 */
void ApplicationManager::unregisterEventHandlers() {
    // TODO: Replace with proper event unsubscription using dearts::EventManager
    // dearts::EventManager::unsubscribe<PluginLoadedEvent>(token);
    // dearts::EventManager::unsubscribe<PluginUnloadedEvent>(token);
}

/**
 * 初始化日志系统
 */
void ApplicationManager::initializeLogging() {
    // 这里可以初始化更复杂的日志系统
    std::cout << "ApplicationManager logging initialized" << std::endl;
}

/**
 * 关闭ImGui
 */
void ApplicationManager::shutdownImGui() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

/**
 * 关闭SDL
 */
void ApplicationManager::shutdownSDL() {
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    SDL_Quit();
}

/**
 * 关闭插件系统
 */
void ApplicationManager::shutdownPlugins() {
    // TODO: PluginManager::unloadAll() method does not exist in the API
    // Consider implementing plugin shutdown logic if needed
    // dearts::PluginManager::unloadAll();
}

} // namespace DearTs::GUI