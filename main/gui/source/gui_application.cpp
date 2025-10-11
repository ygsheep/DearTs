#include "gui_application.h"
#include <chrono>
#include <iostream>
#include <thread>

namespace DearTs {

  /**
   * GUIApplication构造函数
   * 初始化所有成员变量
   */
  GUIApplication::GUIApplication() : m_window(nullptr), m_renderer(nullptr) {}

  /**
   * GUIApplication析构函数
   * 清理所有资源
   */
  GUIApplication::~GUIApplication() { shutdown(); }

  /**
   * 初始化应用程序
   * @param config 应用程序配置
   * @return 是否成功
   */
  bool GUIApplication::initialize(const Core::App::ApplicationConfig &config) {
    try {
      // 1. 调用父类的初始化方法
      if (!Application::initialize(config)) {
        return false;
      }

      // 2. 初始化SDL
      if (!initializeSDL()) {
        throw std::runtime_error("Failed to initialize SDL");
      }

      // 3. 初始化ImGui
      if (!initializeImGui()) {
        throw std::runtime_error("Failed to initialize ImGui");
      }


      std::cout << "GUIApplication initialized successfully" << std::endl;
      return true;

    } catch (const std::exception &e) {
      std::cerr << "GUIApplication initialization failed: " << e.what() << std::endl;
      return false;
    }
  }

  /**
   * 运行应用程序主循环
   * @return 退出代码
   */
  int GUIApplication::run() {
    // 运行主循环直到应用程序请求退出或所有窗口都关闭
    while (getState() != Core::App::ApplicationState::STOPPING && getState() != Core::App::ApplicationState::STOPPED) {
      // 更新应用程序状态
      update(1.0 / m_config.target_fps); // 假设60FPS

      // 检查是否还有窗口存在，如果没有则退出
      auto &windowManager = DearTs::Core::Window::WindowManager::getInstance();
      if (windowManager.getWindowCount() == 0) {
        break;
      }

      // 渲染应用程序界面
      render();

      // 简单的帧率控制
      // std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(m_config.target_fps / 4))); // 约60 FPS
    }

    return 0;
  }

  /**
   * 关闭应用程序
   */
  void GUIApplication::shutdown() {
    std::cout << "Shutting down GUIApplication..." << std::endl;

    // 清理ImGui
    shutdownImGui();

    // 清理资源管理器
    shutdownResourceManager();

    // 清理SDL
    shutdownSDL();

    // 调用父类的关闭方法
    Application::shutdown();

    std::cout << "GUIApplication shutdown complete" << std::endl;
  }

  /**
   * 更新应用程序状态
   * @param delta_time 时间增量（秒）
   */
  void GUIApplication::update(double delta_time) {
    // 处理SDL事件
    processSDLEvents();

    // 更新所有窗口
    auto &windowManager = DearTs::Core::Window::WindowManager::getInstance();
    windowManager.updateAllWindows();

    // 检查并关闭应该关闭的窗口
    if (windowManager.hasWindowsToClose()) {
      windowManager.closeWindowsToClose();
    }

    // 调用父类的更新方法
    Application::update(delta_time);
  }

  /**
   * 渲染应用程序界面
   */
  void GUIApplication::render() {
    // 清屏
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);

    // 开始新帧
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // 渲染主窗口
    if (mainWindow_) {
      mainWindow_->render();
    }

    // 结束帧
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);

    // 呈现
    SDL_RenderPresent(m_renderer);

    // 调用父类的渲染方法
    Application::render();
  }

  /**
   * 处理事件
   * @param event 事件
   */
  void GUIApplication::handleEvent(const Core::Events::Event &event) {
    // 调用父类的事件处理方法
    Application::handleEvent(event);

    // 在这里可以处理特定的事件
  }

  // 私有方法实现

  /**
   * 初始化SDL
   */
  bool GUIApplication::initializeSDL() {
    // 获取窗口管理器实例
    auto &windowManager = DearTs::Core::Window::WindowManager::getInstance();

    // 初始化窗口管理器
    if (!windowManager.initialize()) {
      std::cerr << "Window manager initialization failed" << std::endl;
      return false;
    }

    // 创建主窗口对象
    mainWindow_ = std::make_unique<DearTs::Core::Window::MainWindow>("DearTs GUI Application");
    if (!mainWindow_->initialize()) {
      std::cerr << "Main window initialization failed" << std::endl;
      return false;
    }

    // 获取SDL窗口和渲染器
    auto window = mainWindow_->getWindow();
    if (!window) {
      std::cerr << "Failed to get window from main window" << std::endl;
      return false;
    }

    m_window = window->getSDLWindow();


    // 创建渲染器
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    if (!m_renderer) {
      std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
      return false;
    }

    windowManager.addWindow(mainWindow_->getWindow());

    return true;
  }

  /**
   * 初始化ImGui
   */
  bool GUIApplication::initializeImGui() {
    // 检查ImGui版本
    IMGUI_CHECKVERSION();

    // 创建ImGui上下文
    ImGui::CreateContext();

    // 配置ImGui
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 设置ImGui样式
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();

    // 调整样式以适应高DPI显示器
    style.ScaleAllSizes(1.0f);

    // 设置常用颜色和样式
    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;

    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(4, 3);
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);

    // 初始化字体管理器
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    if (fontManager && !fontManager->initialize()) {
      std::cerr << "Font manager initialization failed" << std::endl;
      // 继续执行，使用默认字体
    }

    // 初始化资源管理器
    auto resourceManager = DearTs::Core::Resource::ResourceManager::getInstance();
    if (resourceManager && !resourceManager->initialize(m_renderer)) {
      std::cerr << "Resource manager initialization failed" << std::endl;
      // 继续执行，资源可能无法加载
    }

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
   * 处理SDL事件
   */
  void GUIApplication::processSDLEvents() {
    // 调用父类的processEvents()来处理所有SDL事件，包括SDL_QUIT
    // DearTs::Core::App::Application::processEvents();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      // 将事件传递给ImGui SDL2绑定
      ImGui_ImplSDL2_ProcessEvent(&event);

      // 转发所有事件给窗口管理器（用于处理标题栏事件）
      DearTs::Core::Window::WindowManager::getInstance().handleSDLEvent(event);

      // 处理SDL事件
      switch (event.type) {
        case SDL_QUIT:
          DEARTS_LOG_INFO("🛑 收到SDL_QUIT事件，准备退出并关闭所有窗口");
          requestExit();
          // 手动关闭所有窗口，确保窗口关闭流程被触发
        {
          auto& window_manager = DearTs::Core::Window::WindowManager::getInstance();
          auto windows = window_manager.getAllWindows();
          for (auto& window : windows) {
            if (window) {
              DEARTS_LOG_INFO("🚪 SDL_QUIT: 正在关闭窗口 ID: " + std::to_string(window->getId()));
              window->close();
            }
          }
        }
          break;

        default:
          break;
      }
    }

    // 检查是否有窗口请求关闭
    auto& window_manager = DearTs::Core::Window::WindowManager::getInstance();
    if (window_manager.hasWindowsToClose()) {
      DEARTS_LOG_INFO("🔍 发现需要关闭的窗口，正在处理...");
      window_manager.closeWindowsToClose();
      if (window_manager.getWindowCount() == 0) {
        DEARTS_LOG_INFO("🏠 所有窗口已关闭，请求退出");
        requestExit();
      }
    }
  }


  /**
   * 更新番茄时钟状态
   */


  /**
   * 关闭ImGui
   */
  void GUIApplication::shutdownImGui() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
  }

  /**
   * 关闭资源管理器
   */
  void GUIApplication::shutdownResourceManager() {
    auto resourceManager = DearTs::Core::Resource::ResourceManager::getInstance();
    if (resourceManager) {
      resourceManager->shutdown();
    }
  }

  /**
   * 关闭SDL
   */
  void GUIApplication::shutdownSDL() {
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

} // namespace DearTs
