/**
 * DearTs Window Manager Implementation
 *
 * 窗口管理器实现文件 - 提供跨平台窗口管理功能实现
 *
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#include "window_manager.h"
#include "../core.h"
#include "window_base.h"
// Logger removed - using simple output instead
#include <SDL_image.h>
#include <algorithm>
#include <stdexcept>
#include "../resource/resource_manager.h"
#include "../utils/file_utils.h"

// Windows特定头文件
#if defined(_WIN32)
#include <SDL_syswm.h>
#include <windows.h>
#endif

namespace DearTs {
  namespace Core {
    namespace Window {

      // ============================================================================
      // 静态成员初始化
      // ============================================================================

      std::atomic<uint32_t> Window::s_nextId{1};

      // ============================================================================
      // Window 实现
      // ============================================================================

      Window::Window(const WindowConfig &config) :
          m_id(s_nextId.fetch_add(1)), m_config(config), m_sdlWindow(nullptr), m_state(WindowState::NORMAL),
          m_shouldClose(false), m_isDragging(false), m_userData(nullptr), m_eventHandler(nullptr) {

        DEARTS_LOG_DEBUG("创建窗口，ID: " + std::to_string(m_id));
        DEARTS_LOG_DEBUG("窗口配置图标路径: " + m_config.icon_path);
      }

      Window::~Window() {
        destroy();
        DEARTS_LOG_DEBUG("销毁窗口，ID: " + std::to_string(m_id));
      }

      bool Window::create() {
        if (m_sdlWindow) {
          DEARTS_LOG_WARN("窗口已创建");
          return true;
        }

        // 创建SDL窗口
        m_sdlWindow = SDL_CreateWindow(m_config.title.c_str(), m_config.position.x, m_config.position.y,
                                       m_config.size.width, m_config.size.height, static_cast<uint32_t>(m_config.flags));

        if (!m_sdlWindow) {
          DEARTS_LOG_ERROR("创建SDL窗口失败: " + std::string(SDL_GetError()));
          return false;
        }

        // 设置窗口大小限制
        if (m_config.min_size.width > 0 && m_config.min_size.height > 0) {
          SDL_SetWindowMinimumSize(m_sdlWindow, m_config.min_size.width, m_config.min_size.height);
        }

        if (m_config.max_size.width > 0 && m_config.max_size.height > 0) {
          SDL_SetWindowMaximumSize(m_sdlWindow, m_config.max_size.width, m_config.max_size.height);
        }

        // 设置窗口图标
        DEARTS_LOG_DEBUG("窗口创建: 检查图标路径: " + m_config.icon_path);
        if (!m_config.icon_path.empty()) {
          DEARTS_LOG_DEBUG("窗口创建: 从路径设置图标: " + m_config.icon_path);
          setIcon(m_config.icon_path);
        } else {
          DEARTS_LOG_DEBUG("窗口创建: 图标路径为空");
        }

        // 初始化渲染器
        if (m_renderer) {
          if (!m_renderer->initialize(m_sdlWindow)) {
            DEARTS_LOG_ERROR("初始化窗口渲染器失败");
            destroy();
            return false;
          }
        }

        // 更新窗口状态
        updateState();

        // 分发窗口创建事件
        dispatchEvent(Events::EventType::EVT_WINDOW_CREATED);

        DEARTS_LOG_INFO("🪟 窗口创建成功: " + m_config.title + " (" + std::to_string(m_config.size.width) + "x" +
                        std::to_string(m_config.size.height) + ")");

        return true;
      }

      void Window::destroy() {
        if (!m_sdlWindow) {
          return;
        }

        // 关闭渲染器
        if (m_renderer) {
          m_renderer->shutdown();
        }

        // 分发窗口销毁事件
        dispatchEvent(Events::EventType::EVT_WINDOW_DESTROYED);

        // 销毁SDL窗口
        SDL_DestroyWindow(m_sdlWindow);
        m_sdlWindow = nullptr;

        m_state = WindowState::CLOSED;

        DEARTS_LOG_INFO("💥 窗口已销毁: " + m_config.title);
      }

      void Window::show() {
        DEARTS_LOG_DEBUG("Window::show 被调用，窗口ID: " + std::to_string(m_id));
        if (m_sdlWindow) {
          DEARTS_LOG_DEBUG("SDL_ShowWindow 被调用");
          SDL_ShowWindow(m_sdlWindow);
          updateState();

          // 通知WindowBase窗口已显示（如果存在）
          if (m_userData) {
            DEARTS_LOG_DEBUG("通知WindowBase窗口已显示");
            auto* windowBase = static_cast<WindowBase*>(m_userData);
            if (windowBase) {
              windowBase->onWindowShown();
            }
          } else {
            DEARTS_LOG_WARN("Window::show: userData 为空");
          }
        } else {
          DEARTS_LOG_ERROR("Window::show: SDL窗口为空");
        }
      }

      void Window::hide() {
        if (m_sdlWindow) {
          SDL_HideWindow(m_sdlWindow);
          m_state = WindowState::HIDDEN;

          // 通知WindowBase窗口已隐藏（如果存在）
          if (m_userData) {
            auto* windowBase = static_cast<WindowBase*>(m_userData);
            if (windowBase) {
              windowBase->onWindowHidden();
            }
          }
        }
      }

      void Window::minimize() {
        if (m_sdlWindow) {
          SDL_MinimizeWindow(m_sdlWindow);
          m_state = WindowState::MINIMIZED;
          dispatchEvent(Events::EventType::EVT_WINDOW_MINIMIZED);
        }
      }

      void Window::maximize() {
        if (m_sdlWindow) {
          SDL_MaximizeWindow(m_sdlWindow);
          m_state = WindowState::MAXIMIZED;
          dispatchEvent(Events::EventType::EVT_WINDOW_MAXIMIZED);
        }
      }

      void Window::restore() {
        if (m_sdlWindow) {
          SDL_RestoreWindow(m_sdlWindow);
          m_state = WindowState::NORMAL;
          dispatchEvent(Events::EventType::EVT_WINDOW_RESTORED);
        }
      }

      void Window::setFullscreen(bool fullscreen, bool desktop_fullscreen) {
        if (!m_sdlWindow) {
          return;
        }

        uint32_t flags = 0;
        if (fullscreen) {
          flags = desktop_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN;
          m_state = WindowState::FULLSCREEN;
        } else {
          m_state = WindowState::NORMAL;
        }

        if (SDL_SetWindowFullscreen(m_sdlWindow, flags) != 0) {
          DEARTS_LOG_ERROR("Failed to set fullscreen mode: " + std::string(SDL_GetError()));
        }
      }

      void Window::close() {
        DEARTS_LOG_INFO("🔒 窗口关闭中: ID " + std::to_string(m_id));
        m_shouldClose = true;
        DEARTS_LOG_INFO("⚠️ 窗口关闭标志已设置: ID " + std::to_string(m_id));
        dispatchEvent(Events::EventType::EVT_WINDOW_CLOSE_REQUESTED);
        DEARTS_LOG_INFO("✅ 窗口关闭流程完成: ID " + std::to_string(m_id));
      }

      void Window::update() {
        if (!m_sdlWindow) {
          return;
        }

        // 先更新状态
        updateState();

        // 然后调用WindowBase的update方法
        if (m_userData) {
          auto* windowBase = static_cast<WindowBase*>(m_userData);
          if (windowBase) {
            windowBase->update();
          }
        }
      }

      void Window::render() {
        if (!m_sdlWindow || !m_renderer) {
          return;
        }

        m_renderer->beginFrame();
        m_renderer->clear(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
        m_renderer->endFrame();
      }


      std::string Window::getTitle() const {
        if (m_sdlWindow) {
          return SDL_GetWindowTitle(m_sdlWindow);
        }
        return m_config.title;
      }

      void Window::setTitle(const std::string &title) {
        m_config.title = title;
        if (m_sdlWindow) {
          SDL_SetWindowTitle(m_sdlWindow, title.c_str());
        }
      }

      WindowPosition Window::getPosition() const {
        if (!m_sdlWindow) {
          return m_config.position;
        }

        int x, y;
        SDL_GetWindowPosition(m_sdlWindow, &x, &y);
        return WindowPosition(x, y);
      }

      void Window::setPosition(const WindowPosition &position) {
        m_config.position = position;
        if (m_sdlWindow) {
          SDL_SetWindowPosition(m_sdlWindow, position.x, position.y);
          dispatchEvent(Events::EventType::EVT_WINDOW_MOVED);
        }
      }

      WindowSize Window::getSize() const {
        if (m_sdlWindow) {
          int w, h;
          SDL_GetWindowSize(m_sdlWindow, &w, &h);
          return WindowSize(w, h);
        }
        return m_config.size;
      }

      void Window::setSize(const WindowSize &size) {
        m_config.size = size;
        if (m_sdlWindow) {
          SDL_SetWindowSize(m_sdlWindow, size.width, size.height);
          dispatchEvent(Events::EventType::EVT_WINDOW_RESIZED);
        }
      }

      WindowSize Window::getMinSize() const {
        if (m_sdlWindow) {
          int w, h;
          SDL_GetWindowMinimumSize(m_sdlWindow, &w, &h);
          return WindowSize(w, h);
        }
        return m_config.min_size;
      }

      void Window::setMinSize(const WindowSize &size) {
        m_config.min_size = size;
        if (m_sdlWindow) {
          SDL_SetWindowMinimumSize(m_sdlWindow, size.width, size.height);
        }
      }

      WindowSize Window::getMaxSize() const {
        if (m_sdlWindow) {
          int w, h;
          SDL_GetWindowMaximumSize(m_sdlWindow, &w, &h);
          return WindowSize(w, h);
        }
        return m_config.max_size;
      }

      void Window::setMaxSize(const WindowSize &size) {
        m_config.max_size = size;
        if (m_sdlWindow) {
          SDL_SetWindowMaximumSize(m_sdlWindow, size.width, size.height);
        }
      }

      WindowFlags Window::getFlags() const {
        if (m_sdlWindow) {
          return static_cast<WindowFlags>(SDL_GetWindowFlags(m_sdlWindow));
        }
        return m_config.flags;
      }

      bool Window::setIcon(const std::string &icon_path) {
        if (!m_sdlWindow || icon_path.empty()) {
          DEARTS_LOG_DEBUG("setIcon: Window or icon path is invalid");
          return false;
        }

        DEARTS_LOG_DEBUG("setIcon: Attempting to load icon from: " + icon_path);

        // 使用资源管理器加载图标
        auto resourceManager = DearTs::Core::Resource::ResourceManager::getInstance();
        auto surfaceResource = resourceManager->getSurface(icon_path);

        if (!surfaceResource) {
          DEARTS_LOG_ERROR("Failed to load icon: " + icon_path);
          return false;
        }

        // 获取SDL表面
        SDL_Surface *icon = surfaceResource->getSurface();
        if (!icon) {
          DEARTS_LOG_ERROR("Invalid surface for icon: " + icon_path);
          return false;
        }

        DEARTS_LOG_DEBUG("setIcon: Surface loaded successfully, setting window icon");

        // 设置窗口图标
        SDL_SetWindowIcon(m_sdlWindow, icon);

        m_config.icon_path = icon_path;

        DEARTS_LOG_DEBUG("Window icon set: " + icon_path);
        return true;
      }

      float Window::getOpacity() const {
        if (m_sdlWindow) {
          float opacity;
          if (SDL_GetWindowOpacity(m_sdlWindow, &opacity) == 0) {
            return opacity;
          }
        }
        return 1.0f;
      }

      void Window::setOpacity(float opacity) const {
        if (m_sdlWindow) {
          opacity = std::clamp(opacity, 0.0f, 1.0f);
          if (SDL_SetWindowOpacity(m_sdlWindow, opacity) != 0) {
            DEARTS_LOG_ERROR("Failed to set window opacity: " + std::string(SDL_GetError()));
          }
        }
      }

      bool Window::isVisible() const {
        if (m_sdlWindow) {
          const uint32_t flags = SDL_GetWindowFlags(m_sdlWindow);
          return (flags & SDL_WINDOW_SHOWN) != 0;
        }
        return false;
      }

      bool Window::hasFocus() const {
        if (m_sdlWindow) {
          const uint32_t flags = SDL_GetWindowFlags(m_sdlWindow);
          return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
        }
        return false;
      }

      int Window::getDisplayIndex() const {
        if (m_sdlWindow) {
          const int index = SDL_GetWindowDisplayIndex(m_sdlWindow);
          return index >= 0 ? index : 0;
        }
        return m_config.display_index;
      }

      float Window::getDPIScale() const {
        if (m_sdlWindow) {
          const int display_index = getDisplayIndex();
          float ddpi, hdpi, vdpi;
          if (SDL_GetDisplayDPI(display_index, &ddpi, &hdpi, &vdpi) == 0) {
            return ddpi / 96.0f; // 96 DPI是标准DPI
          }
        }
        return 1.0f;
      }

      void Window::setRenderer(std::unique_ptr<WindowRenderer> renderer) {
        if (m_renderer) {
          m_renderer->shutdown();
        }

        m_renderer = std::move(renderer);

        if (m_renderer && m_sdlWindow) {
          if (!m_renderer->initialize(m_sdlWindow)) {
            DEARTS_LOG_ERROR("Failed to initialize new renderer");
            m_renderer.reset();
          }
        }
      }

      void Window::setEventHandler(std::shared_ptr<WindowEventHandler> handler) { m_eventHandler = handler; }

      void Window::handleSDLEvent(const SDL_Event &event) {
        switch (event.type) {
          case SDL_WINDOWEVENT:
            if (event.window.windowID == SDL_GetWindowID(m_sdlWindow)) {
              switch (event.window.event) {
                case SDL_WINDOWEVENT_CLOSE:
                  if (m_eventHandler && m_eventHandler->onWindowClose(this)) {
                    close();
                  }
                  break;

                case SDL_WINDOWEVENT_RESIZED:
                  if (m_eventHandler) {
                    m_eventHandler->onWindowResize(this, event.window.data1, event.window.data2);
                  }
                  dispatchEvent(Events::EventType::EVT_WINDOW_RESIZED);
                  break;

                case SDL_WINDOWEVENT_MOVED:
                  if (m_eventHandler) {
                    m_eventHandler->onWindowMove(this, event.window.data1, event.window.data2);
                  }
                  dispatchEvent(Events::EventType::EVT_WINDOW_MOVED);
                  break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                  if (m_eventHandler) {
                    m_eventHandler->onWindowFocusGained(this);
                  }
                  dispatchEvent(Events::EventType::EVT_WINDOW_FOCUS_GAINED);
                  break;

                case SDL_WINDOWEVENT_FOCUS_LOST:
                  if (m_eventHandler) {
                    m_eventHandler->onWindowFocusLost(this);
                  }
                  dispatchEvent(Events::EventType::EVT_WINDOW_FOCUS_LOST);
                  break;

                case SDL_WINDOWEVENT_MINIMIZED:
                  m_state = WindowState::MINIMIZED;
                  if (m_eventHandler) {
                    m_eventHandler->onWindowMinimized(this);
                  }
                  dispatchEvent(Events::EventType::EVT_WINDOW_MINIMIZED);
                  break;

                case SDL_WINDOWEVENT_MAXIMIZED:
                  m_state = WindowState::MAXIMIZED;
                  if (m_eventHandler) {
                    m_eventHandler->onWindowMaximized(this);
                  }
                  dispatchEvent(Events::EventType::EVT_WINDOW_MAXIMIZED);
                  break;

                case SDL_WINDOWEVENT_RESTORED:
                  m_state = WindowState::NORMAL;
                  if (m_eventHandler) {
                    m_eventHandler->onWindowRestored(this);
                  }
                  dispatchEvent(Events::EventType::EVT_WINDOW_RESTORED);
                  break;

                case SDL_WINDOWEVENT_SHOWN:
                  if (m_eventHandler) {
                    m_eventHandler->onWindowShown(this);
                  }
                  break;

                case SDL_WINDOWEVENT_HIDDEN:
                  m_state = WindowState::HIDDEN;
                  if (m_eventHandler) {
                    m_eventHandler->onWindowHidden(this);
                  }
                  break;

                case SDL_WINDOWEVENT_EXPOSED:
                  if (m_eventHandler) {
                    m_eventHandler->onWindowExposed(this);
                  }
                  break;
                default:
                  break;
              }
            }
            break;

          // 处理鼠标事件，传递给 WindowBase
          case SDL_MOUSEBUTTONDOWN:
          case SDL_MOUSEBUTTONUP:
          case SDL_MOUSEMOTION:
          case SDL_MOUSEWHEEL:
            // 将鼠标事件传递给 WindowBase 处理
            if (m_userData) {
              auto *windowBase = static_cast<WindowBase *>(m_userData);
              windowBase->handleEvent(event);
            }
            break;
        }
      }

      void Window::updateState() {
        if (!m_sdlWindow) {
          return;
        }

        uint32_t flags = SDL_GetWindowFlags(m_sdlWindow);

        if (flags & SDL_WINDOW_MINIMIZED) {
          m_state = WindowState::MINIMIZED;
        } else if (flags & SDL_WINDOW_MAXIMIZED) {
          m_state = WindowState::MAXIMIZED;
        } else if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
          m_state = WindowState::FULLSCREEN;
        } else if (!(flags & SDL_WINDOW_SHOWN)) {
          m_state = WindowState::HIDDEN;
        } else {
          m_state = WindowState::NORMAL;
        }
      }

      void Window::dispatchEvent(::DearTs::Core::Events::EventType type) {
        // 这里可以创建具体的窗口事件并分发
        // 目前只是记录日志
        DEARTS_LOG_DEBUG("Window event dispatched: " + std::to_string(static_cast<uint32_t>(type)) + " for window " +
                         std::to_string(m_id));
      }

      // ============================================================================
      // WindowManager 实现
      // ============================================================================

      WindowManager &WindowManager::getInstance() {
        static WindowManager instance;
        return instance;
      }

      bool WindowManager::initialize() {
        if (m_initialized) {
          return true;
        }

        // 初始化SDL视频子系统
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
          DEARTS_LOG_ERROR("初始化SDL视频子系统失败: " + std::string(SDL_GetError()));
          return false;
        }

        // 初始化SDL_image
        int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
        if ((IMG_Init(img_flags) & img_flags) != img_flags) {
          DEARTS_LOG_WARN("初始化SDL_image失败: " + std::string(IMG_GetError()));
        }

        // 设置默认配置
        m_defaultConfig = WindowConfig();
        m_globalVSync = true;

        m_initialized = true;

        DEARTS_LOG_INFO("🖼️ 窗口管理器初始化成功！");
        return true;
      }

      void WindowManager::shutdown() {
        if (!m_initialized) {
          return;
        }

        // 销毁所有窗口
        {
          std::lock_guard<std::mutex> lock(m_windowsMutex);
          for (auto& [id, window] : m_windows) {
            if (window) {
              window->destroy();
            }
          }
          m_windows.clear();
        }

        // 关闭SDL_image
        IMG_Quit();

        // 关闭SDL视频子系统
        SDL_QuitSubSystem(SDL_INIT_VIDEO);

        m_initialized = false;

        DEARTS_LOG_INFO("🔒 窗口管理器已关闭");
      }

      std::shared_ptr<Window> WindowManager::createWindow(const WindowConfig &config) {
        if (!m_initialized) {
          DEARTS_LOG_ERROR("Window manager not initialized");
          return nullptr;
        }

        auto window = std::make_shared<Window>(config);
        if (!window->create()) {
          DEARTS_LOG_ERROR("Failed to create window");
          return nullptr;
        }

        // 标题栏初始化由具体窗口类处理（如MainWindow）
        // 这里不再调用window->initializeTitleBar()

        {
          std::lock_guard<std::mutex> lock(m_windowsMutex);
          m_windows[window->getId()] = window;
        }

        DEARTS_LOG_INFO("✨ 新窗口已创建: " + config.title + " (ID: " + std::to_string(window->getId()) + ")");
        return window;
      }

      bool WindowManager::addWindow(std::shared_ptr<Window> window) {
        if (!m_initialized) {
          DEARTS_LOG_ERROR("Window manager not initialized");
          return false;
        }

        if (!window) {
          DEARTS_LOG_ERROR("Window is null");
          return false;
        }

        // 标题栏初始化由具体窗口类处理（如MainWindow）
        // 这里不再调用window->initializeTitleBar()

        {
          std::lock_guard<std::mutex> lock(m_windowsMutex);
          m_windows[window->getId()] = window;
        }

        DEARTS_LOG_INFO("➕ 窗口已添加: " + window->getTitle() + " (ID: " + std::to_string(window->getId()) + ")");
        return true;
      }

      void WindowManager::destroyWindow(uint32_t window_id) {
        DEARTS_LOG_INFO("🗑️ 窗口管理器：准备销毁窗口 ID: " + std::to_string(window_id));
        std::lock_guard<std::mutex> lock(m_windowsMutex);

        auto it = m_windows.find(window_id);
        if (it != m_windows.end()) {
          if (it->second) {
            DEARTS_LOG_INFO("🔥 正在销毁窗口 ID: " + std::to_string(window_id));
            it->second->destroy();

            // 从命名映射中移除
            for (auto named_it = m_namedWindows.begin(); named_it != m_namedWindows.end(); ) {
              if (named_it->second && named_it->second->getId() == window_id) {
                DEARTS_LOG_INFO("🗑️ 从命名映射中移除窗口: " + named_it->first);
                named_it = m_namedWindows.erase(named_it);
              } else {
                ++named_it;
              }
            }
          }
          m_windows.erase(it);

          DEARTS_LOG_INFO("✅ 窗口已销毁，ID: " + std::to_string(window_id));
        } else {
          DEARTS_LOG_WARN("Window not found for destruction: ID " + std::to_string(window_id));
        }
      }

      void WindowManager::destroyWindow(std::shared_ptr<Window> window) {
        if (window) {
          destroyWindow(window->getId());
        }
      }

      std::shared_ptr<Window> WindowManager::getWindow(uint32_t window_id) const {
        std::lock_guard<std::mutex> lock(m_windowsMutex);

        auto it = m_windows.find(window_id);
        return (it != m_windows.end()) ? it->second : nullptr;
      }

      std::shared_ptr<Window> WindowManager::getWindowBySDLId(uint32_t sdl_window_id) const {
        std::lock_guard<std::mutex> lock(m_windowsMutex);

        for (const auto& [id, window] : m_windows) {
          if (window && window->getSDLWindow()) {
            if (SDL_GetWindowID(window->getSDLWindow()) == sdl_window_id) {
              return window;
            }
          }
        }

        return nullptr;
      }

      std::vector<std::shared_ptr<Window>> WindowManager::getAllWindows() const {
        std::lock_guard<std::mutex> lock(m_windowsMutex);

        std::vector<std::shared_ptr<Window>> result;
        result.reserve(m_windows.size());

        for (const auto& [id, window] : m_windows) {
          if (window) {
            result.push_back(window);
          }
        }

        return result;
      }

      size_t WindowManager::getWindowCount() const {
        std::lock_guard<std::mutex> lock(m_windowsMutex);
        return m_windows.size();
      }

      void WindowManager::updateAllWindows() {
        auto windows = getAllWindows();
        for (auto& window : windows) {
          if (window && window->isCreated()) {
            window->update();
          }
        }
      }


      void WindowManager::renderAllWindows() {
        auto windows = getAllWindows();

        // 检查是否有窗口正在拖拽
        bool any_window_dragging = false;
        for (const auto &w: windows) {
          if (w && w->isDragging()) {
            any_window_dragging = true;
            break;
          }
        }

        // 如果有窗口正在拖拽，降低整体渲染频率
        static auto last_render_time = std::chrono::steady_clock::now();
        if (any_window_dragging) {
          auto now = std::chrono::steady_clock::now();
          auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_render_time);

          // 在拖拽过程中降低渲染频率到30FPS
          if (elapsed.count() < 33) { // 33ms ≈ 30FPS
            return;
          }

          last_render_time = now;
        } else {
          last_render_time = std::chrono::steady_clock::now();
        }

        for (auto &window: windows) {
          if (window && window->isCreated() && window->isVisible()) {
            // 检查SDL窗口是否仍然有效
            if (!window->getSDLWindow()) {
              DEARTS_LOG_WARN("窗口的SDL窗口无效，跳过渲染");
              continue;
            }

            // 检查是否是SDLRenderer并开始ImGui帧
            auto renderer = window->getRenderer();

            // 渲染ImGui（如果使用渲染器）
            if (renderer) {

              // 获取SDLRenderer来调用newImGuiFrame和renderImGui方法
              // 检查渲染器类型
              auto sdlRenderer = dynamic_cast<DearTs::Core::Render::SDLRenderer *>(renderer);

              // 检查是否是适配器，如果是则尝试获取底层渲染器
              if (!sdlRenderer) {
                auto adapter = dynamic_cast<DearTs::Core::Render::IRendererToWindowRendererAdapter *>(renderer);
                if (adapter) {
                  // 从适配器中获取底层渲染器
                  auto underlyingRenderer = adapter->getRenderer();
                  if (underlyingRenderer) {
                    sdlRenderer = dynamic_cast<DearTs::Core::Render::SDLRenderer *>(underlyingRenderer.get());
                  }
                }
              }

              if (sdlRenderer) {
                // 开始ImGui帧
                sdlRenderer->newImGuiFrame();

                // 清除屏幕背景（在ImGui帧开始后清除，避免重影）
                // 注意：这个清除操作应该在ImGui渲染之前进行，以避免覆盖ImGui内容
                // 使用颜色值(36,36,36)更接近用户指出的遮挡标题栏的颜色
                sdlRenderer->clear(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

                // 标题栏渲染由具体窗口类处理（如MainWindow）
                // 这里不再调用window->renderTitleBar()

                // 渲染窗口内容（调用WindowBase的render方法）
                if (!any_window_dragging) {
                  // 只在非拖拽时渲染复杂内容
                  if (window->getUserData()) {
                    auto* windowBase = static_cast<WindowBase*>(window->getUserData());
                    if (windowBase) {
                      windowBase->render();
                    }
                  }
                }

                // 渲染ImGui
                ImGui::Render();

                ImDrawData *draw_data = ImGui::GetDrawData();
                sdlRenderer->renderImGui(draw_data);

                // 呈现
                sdlRenderer->present();
              } else {
                // 如果不是SDLRenderer，使用适配器渲染器
                renderer->beginFrame();

                // 清空屏幕为深色背景
                // 使用颜色值(36,36,36)更接近用户指出的遮挡标题栏的颜色
                renderer->clear(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);

                // 呈现
                renderer->endFrame();
                renderer->present();
              }
            } else {
              // 如果没有渲染器，仍然渲染窗口内容
              window->render();
            }
          }
        }
        // renderAllWindows完成
      }

      void WindowManager::handleSDLEvent(const SDL_Event &event) {
        
        // 只对重要事件记录日志，避免频繁输出
        if (event.type == SDL_WINDOWEVENT || event.type == SDL_QUIT) {
          DEARTS_LOG_DEBUG("WindowManager处理事件，类型: " + std::to_string(event.type));
        }

        // 处理窗口事件
        // if (event.type == SDL_WINDOWEVENT) {
        //   auto window = getWindowBySDLId(event.window.windowID);
        //   if (window) {
        //     window->handleSDLEvent(event);
        //   }
        // }
        // // 处理鼠标事件（用于标题栏拖拽等）
        // else if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN ||
        //          event.type == SDL_MOUSEBUTTONUP) {
        //   // 将鼠标事件传递给所有窗口，让它们自己判断是否处理
        //   auto windows = getAllWindows();
        //   for (auto &window: windows) {
        //     if (window) {
        //       window->handleSDLEvent(event);
        //     }
        //   }
        // }

        // 标题栏事件处理由具体窗口类处理
        // 这里不再调用window->handleTitleBarEvent(event)
        auto window = getWindowBySDLId(event.window.windowID);
        if (window) {
          window->handleSDLEvent(event);
        }
      }

      bool WindowManager::hasWindowsToClose() const {
        auto windows = getAllWindows();
        DEARTS_LOG_DEBUG("Checking hasWindowsToClose, window count: " + std::to_string(windows.size()));

        bool result = std::any_of(windows.begin(), windows.end(), [](const std::shared_ptr<Window> &window) {
          bool should_close = window && window->shouldClose();
          if (window) {
            DEARTS_LOG_DEBUG("Window ID " + std::to_string(window->getId()) +
                             " shouldClose: " + std::to_string(should_close));
          }
          return should_close;
        });

        DEARTS_LOG_DEBUG("hasWindowsToClose result: " + std::to_string(result));
        return result;
      }

      void WindowManager::closeWindowsToClose() {
        DEARTS_LOG_INFO("🔍 检查需要关闭的窗口...");
        auto windows = getAllWindows();
        DEARTS_LOG_INFO("📊 找到 " + std::to_string(windows.size()) + " 个窗口需要检查");

        int closed_count = 0;
        for (auto &window: windows) {
          if (window && window->shouldClose()) {
            DEARTS_LOG_INFO("🚪 正在关闭窗口 ID: " + std::to_string(window->getId()));
            destroyWindow(window->getId());
            closed_count++;
          }
        }

        DEARTS_LOG_INFO("✅ 已关闭 " + std::to_string(closed_count) + " 个窗口");
      }

      int WindowManager::getDisplayCount() const { return SDL_GetNumVideoDisplays(); }

      DisplayInfo WindowManager::getDisplayInfo(int display_index) const {
        DisplayInfo info{};
        info.index = display_index;

        // 获取显示器名称
        const char *name = SDL_GetDisplayName(display_index);
        info.name = name ? name : "Unknown Display";

        // 获取显示器边界
        if (SDL_GetDisplayBounds(display_index, &info.bounds) != 0) {
          DEARTS_LOG_ERROR("Failed to get display bounds: " + std::string(SDL_GetError()));
        }

        // 获取可用区域
        if (SDL_GetDisplayUsableBounds(display_index, &info.usable_bounds) != 0) {
          info.usable_bounds = info.bounds;
        }

        // 获取DPI
        float ddpi, hdpi, vdpi;
        if (SDL_GetDisplayDPI(display_index, &ddpi, &hdpi, &vdpi) == 0) {
          info.dpi = ddpi;
        } else {
          info.dpi = 96.0f; // 默认DPI
        }

        // 获取显示模式
        SDL_DisplayMode mode;
        if (SDL_GetCurrentDisplayMode(display_index, &mode) == 0) {
          info.refresh_rate = mode.refresh_rate;
          info.pixel_format = SDL_AllocFormat(mode.format);
        } else {
          info.refresh_rate = 60; // 默认刷新率
          info.pixel_format = nullptr;
        }

        // 检查是否为主显示器
        info.is_primary = (display_index == 0);

        return info;
      }

      std::vector<DisplayInfo> WindowManager::getAllDisplays() const {
        std::vector<DisplayInfo> displays;
        int count = getDisplayCount();

        displays.reserve(count);
        for (int i = 0; i < count; ++i) {
          displays.push_back(getDisplayInfo(i));
        }

        return displays;
      }

      DisplayInfo WindowManager::getPrimaryDisplay() const { return getDisplayInfo(0); }

      void WindowManager::setGlobalVSync(bool enabled) {
        m_globalVSync = enabled;
        DEARTS_LOG_INFO("🎮 垂直同步设置: " + std::string(enabled ? "已启用 🟢" : "已禁用 🔴"));
      }

      void WindowManager::setDefaultWindowConfig(const WindowConfig &config) {
        m_defaultConfig = config;
        DEARTS_LOG_DEBUG("Default window config updated");
      }

      bool WindowManager::addWindow(const std::string& name, std::shared_ptr<Window> window) {
        if (!m_initialized) {
          DEARTS_LOG_ERROR("Window manager not initialized");
          return false;
        }

        if (!window) {
          DEARTS_LOG_ERROR("Window is null");
          return false;
        }

        {
          std::lock_guard<std::mutex> lock(m_windowsMutex);

          // 添加到按ID的映射
          m_windows[window->getId()] = window;

          // 添加到按名称的映射
          m_namedWindows[name] = window;
        }

        DEARTS_LOG_INFO("➕ 窗口已添加: " + window->getTitle() + " (名称: " + name + ", ID: " + std::to_string(window->getId()) + ")");
        return true;
      }

      std::shared_ptr<Window> WindowManager::getWindowByName(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_windowsMutex);

        auto it = m_namedWindows.find(name);
        return (it != m_namedWindows.end()) ? it->second : nullptr;
      }

      bool WindowManager::showWindow(const std::string& name) {
        DEARTS_LOG_DEBUG("WindowManager::showWindow 被调用: " + name);
        auto window = getWindowByName(name);
        if (window) {
          DEARTS_LOG_DEBUG("找到窗口对象，准备显示: " + name);
          window->show();
          DEARTS_LOG_INFO("👁️ 窗口已显示: " + name);
          return true;
        } else {
          DEARTS_LOG_ERROR("找不到窗口: " + name);
          return false;
        }
      }

      bool WindowManager::hideWindow(const std::string& name) {
        auto window = getWindowByName(name);
        if (window) {
          window->hide();
          DEARTS_LOG_INFO("🙈 窗口已隐藏: " + name);
          return true;
        } else {
          DEARTS_LOG_ERROR("找不到窗口: " + name);
          return false;
        }
      }

      bool WindowManager::toggleWindow(const std::string& name) {
        auto window = getWindowByName(name);
        if (window) {
          if (window->isVisible()) {
            window->hide();
            DEARTS_LOG_INFO("🙈 窗口已隐藏: " + name);
          } else {
            window->show();
            DEARTS_LOG_INFO("👁️ 窗口已显示: " + name);
          }
          return true;
        } else {
          DEARTS_LOG_ERROR("找不到窗口: " + name);
          return false;
        }
      }

      bool WindowManager::isWindowVisible(const std::string& name) const {
        auto window = getWindowByName(name);
        return window ? window->isVisible() : false;
      }

      bool WindowManager::focusWindow(const std::string& name) {
        auto window = getWindowByName(name);
        if (window && window->getSDLWindow()) {
          SDL_RaiseWindow(window->getSDLWindow());
          DEARTS_LOG_INFO("🎯 窗口已获得焦点: " + name);
          return true;
        } else {
          DEARTS_LOG_ERROR("找不到窗口或窗口无效: " + name);
          return false;
        }
      }

    } // namespace Window
  } // namespace Core
} // namespace DearTs
