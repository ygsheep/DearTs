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
// Logger removed - using simple output instead
#include "../utils/file_utils.h"
#include "../resource/resource_manager.h"
#include <SDL_image.h>
#include <algorithm>
#include <stdexcept>

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

std::atomic<uint32_t> Window::next_id_{1};

// ============================================================================
// Window 实现
// ============================================================================

Window::Window(const WindowConfig& config)
    : id_(next_id_.fetch_add(1))
    , config_(config)
    , sdl_window_(nullptr)
    , state_(WindowState::NORMAL)
    , should_close_(false)
    , is_dragging_(false)
    , user_data_(nullptr)
    , event_handler_(nullptr) {
    
    DEARTS_LOG_DEBUG("创建窗口，ID: " + std::to_string(id_));
    DEARTS_LOG_DEBUG("窗口配置图标路径: " + config_.icon_path);
}

Window::~Window() {
    destroy();
    DEARTS_LOG_DEBUG("销毁窗口，ID: " + std::to_string(id_));
}

bool Window::create() {
    if (sdl_window_) {
        DEARTS_LOG_WARN("窗口已创建");
        return true;
    }
    
    // 创建SDL窗口
    sdl_window_ = SDL_CreateWindow(
        config_.title.c_str(),
        config_.position.x,
        config_.position.y,
        config_.size.width,
        config_.size.height,
        static_cast<uint32_t>(config_.flags)
    );
    
    if (!sdl_window_) {
        DEARTS_LOG_ERROR("创建SDL窗口失败: " + std::string(SDL_GetError()));
        return false;
    }
    
    // 设置窗口大小限制
    if (config_.min_size.width > 0 && config_.min_size.height > 0) {
        SDL_SetWindowMinimumSize(sdl_window_, config_.min_size.width, config_.min_size.height);
    }
    
    if (config_.max_size.width > 0 && config_.max_size.height > 0) {
        SDL_SetWindowMaximumSize(sdl_window_, config_.max_size.width, config_.max_size.height);
    }
    
    // 设置窗口图标
    DEARTS_LOG_DEBUG("窗口创建: 检查图标路径: " + config_.icon_path);
    if (!config_.icon_path.empty()) {
        DEARTS_LOG_DEBUG("窗口创建: 从路径设置图标: " + config_.icon_path);
        setIcon(config_.icon_path);
    } else {
        DEARTS_LOG_DEBUG("窗口创建: 图标路径为空");
    }
    
    // 初始化渲染器
    if (renderer_) {
        if (!renderer_->initialize(sdl_window_)) {
            DEARTS_LOG_ERROR("初始化窗口渲染器失败");
            destroy();
            return false;
        }
    }
    
    // 更新窗口状态
    updateState();
    
    // 分发窗口创建事件
    dispatchEvent(Events::EventType::EVT_WINDOW_CREATED);
    
    DEARTS_LOG_INFO("窗口创建成功: " + config_.title + " (" + std::to_string(config_.size.width) + "x" + std::to_string(config_.size.height) + ")");
    
    return true;
}

void Window::destroy() {
    if (!sdl_window_) {
        return;
    }
    
    // 关闭渲染器
    if (renderer_) {
        renderer_->shutdown();
    }
    
    // 分发窗口销毁事件
    dispatchEvent(Events::EventType::EVT_WINDOW_DESTROYED);
    
    // 销毁SDL窗口
    SDL_DestroyWindow(sdl_window_);
    sdl_window_ = nullptr;
    
    state_ = WindowState::CLOSED;
    
    DEARTS_LOG_INFO("Window destroyed: " + config_.title);
}

void Window::show() {
    if (sdl_window_) {
        SDL_ShowWindow(sdl_window_);
        updateState();
    }
}

void Window::hide() {
    if (sdl_window_) {
        SDL_HideWindow(sdl_window_);
        state_ = WindowState::HIDDEN;
    }
}

void Window::minimize() {
    if (sdl_window_) {
        SDL_MinimizeWindow(sdl_window_);
        state_ = WindowState::MINIMIZED;
        dispatchEvent(Events::EventType::EVT_WINDOW_MINIMIZED);
    }
}

void Window::maximize() {
    if (sdl_window_) {
        SDL_MaximizeWindow(sdl_window_);
        state_ = WindowState::MAXIMIZED;
        dispatchEvent(Events::EventType::EVT_WINDOW_MAXIMIZED);
    }
}

void Window::restore() {
    if (sdl_window_) {
        SDL_RestoreWindow(sdl_window_);
        state_ = WindowState::NORMAL;
        dispatchEvent(Events::EventType::EVT_WINDOW_RESTORED);
    }
}

void Window::setFullscreen(bool fullscreen, bool desktop_fullscreen) {
    if (!sdl_window_) {
        return;
    }
    
    uint32_t flags = 0;
    if (fullscreen) {
        flags = desktop_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN;
        state_ = WindowState::FULLSCREEN;
    } else {
        state_ = WindowState::NORMAL;
    }
    
    if (SDL_SetWindowFullscreen(sdl_window_, flags) != 0) {
        DEARTS_LOG_ERROR("Failed to set fullscreen mode: " + std::string(SDL_GetError()));
    }
}

void Window::close() {
    DEARTS_LOG_INFO("Window::close() called for window ID: " + std::to_string(id_));
    should_close_ = true;
    DEARTS_LOG_INFO("Window::close() - should_close_ set to true for window ID: " + std::to_string(id_));
    dispatchEvent(Events::EventType::EVT_WINDOW_CLOSE_REQUESTED);
    DEARTS_LOG_INFO("Window::close() completed for window ID: " + std::to_string(id_));
}

void Window::update() {
    DEARTS_LOG_DEBUG("Window::update() called for window ID: " + std::to_string(id_));
    if (!sdl_window_) {
        DEARTS_LOG_DEBUG("Window::update() - sdl_window_ is null");
        return;
    }
    
    updateState();
    DEARTS_LOG_DEBUG("Window::update() completed for window ID: " + std::to_string(id_));
}

void Window::render() {
    DEARTS_LOG_DEBUG("Window::render() called for window ID: " + std::to_string(id_));
    if (!sdl_window_ || !renderer_) {
        DEARTS_LOG_DEBUG("Window::render() - sdl_window_ or renderer_ is null");
        return;
    }
    
    renderer_->beginFrame();
    
    // 清空屏幕为深色背景
    // 使用颜色值(36,36,36)更接近用户指出的遮挡标题栏的颜色
    renderer_->clear(36.0f/255.0f, 36.0f/255.0f, 36.0f/255.0f, 1.0f);
    
    // 如果ImGui已初始化，可以在这里添加一些基本的ImGui界面
    // 但实际的ImGui渲染会在WindowManager::renderAllWindows中处理
    
    renderer_->endFrame();
    DEARTS_LOG_DEBUG("Window::render() completed for window ID: " + std::to_string(id_));
}







std::string Window::getTitle() const {
    if (sdl_window_) {
        return SDL_GetWindowTitle(sdl_window_);
    }
    return config_.title;
}

void Window::setTitle(const std::string& title) {
    config_.title = title;
    if (sdl_window_) {
        SDL_SetWindowTitle(sdl_window_, title.c_str());
    }
}

WindowPosition Window::getPosition() const {
    if (sdl_window_) {
        int x, y;
        SDL_GetWindowPosition(sdl_window_, &x, &y);
        return WindowPosition(x, y);
    }
    return config_.position;
}

void Window::setPosition(const WindowPosition& position) {
    config_.position = position;
    if (sdl_window_) {
        SDL_SetWindowPosition(sdl_window_, position.x, position.y);
        dispatchEvent(Events::EventType::EVT_WINDOW_MOVED);
    }
}

WindowSize Window::getSize() const {
    if (sdl_window_) {
        int w, h;
        SDL_GetWindowSize(sdl_window_, &w, &h);
        return WindowSize(w, h);
    }
    return config_.size;
}

void Window::setSize(const WindowSize& size) {
    config_.size = size;
    if (sdl_window_) {
        SDL_SetWindowSize(sdl_window_, size.width, size.height);
        dispatchEvent(Events::EventType::EVT_WINDOW_RESIZED);
    }
}

WindowSize Window::getMinSize() const {
    if (sdl_window_) {
        int w, h;
        SDL_GetWindowMinimumSize(sdl_window_, &w, &h);
        return WindowSize(w, h);
    }
    return config_.min_size;
}

void Window::setMinSize(const WindowSize& size) {
    config_.min_size = size;
    if (sdl_window_) {
        SDL_SetWindowMinimumSize(sdl_window_, size.width, size.height);
    }
}

WindowSize Window::getMaxSize() const {
    if (sdl_window_) {
        int w, h;
        SDL_GetWindowMaximumSize(sdl_window_, &w, &h);
        return WindowSize(w, h);
    }
    return config_.max_size;
}

void Window::setMaxSize(const WindowSize& size) {
    config_.max_size = size;
    if (sdl_window_) {
        SDL_SetWindowMaximumSize(sdl_window_, size.width, size.height);
    }
}

WindowFlags Window::getFlags() const {
    if (sdl_window_) {
        return static_cast<WindowFlags>(SDL_GetWindowFlags(sdl_window_));
    }
    return config_.flags;
}

bool Window::setIcon(const std::string& icon_path) {
    if (!sdl_window_ || icon_path.empty()) {
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
    SDL_Surface* icon = surfaceResource->getSurface();
    if (!icon) {
        DEARTS_LOG_ERROR("Invalid surface for icon: " + icon_path);
        return false;
    }
    
    DEARTS_LOG_DEBUG("setIcon: Surface loaded successfully, setting window icon");
    
    // 设置窗口图标
    SDL_SetWindowIcon(sdl_window_, icon);
    
    config_.icon_path = icon_path;
    
    DEARTS_LOG_DEBUG("Window icon set: " + icon_path);
    return true;
}

float Window::getOpacity() const {
    if (sdl_window_) {
        float opacity;
        if (SDL_GetWindowOpacity(sdl_window_, &opacity) == 0) {
            return opacity;
        }
    }
    return 1.0f;
}

void Window::setOpacity(float opacity) const {
    if (sdl_window_) {
        opacity = std::clamp(opacity, 0.0f, 1.0f);
        if (SDL_SetWindowOpacity(sdl_window_, opacity) != 0) {
            DEARTS_LOG_ERROR("Failed to set window opacity: " + std::string(SDL_GetError()));
        }
    }
}

bool Window::isVisible() const {
    if (sdl_window_) {
        const uint32_t flags = SDL_GetWindowFlags(sdl_window_);
        return (flags & SDL_WINDOW_SHOWN) != 0;
    }
    return false;
}

bool Window::hasFocus() const {
    if (sdl_window_) {
        const uint32_t flags = SDL_GetWindowFlags(sdl_window_);
        return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
    }
    return false;
}

int Window::getDisplayIndex() const {
    if (sdl_window_) {
        const int index = SDL_GetWindowDisplayIndex(sdl_window_);
        return index >= 0 ? index : 0;
    }
    return config_.display_index;
}

float Window::getDPIScale() const {
    if (sdl_window_) {
        const int display_index = getDisplayIndex();
        float ddpi, hdpi, vdpi;
        if (SDL_GetDisplayDPI(display_index, &ddpi, &hdpi, &vdpi) == 0) {
            return ddpi / 96.0f; // 96 DPI是标准DPI
        }
    }
    return 1.0f;
}

void Window::setRenderer(std::unique_ptr<WindowRenderer> renderer) {
    if (renderer_) {
        renderer_->shutdown();
    }
    
    renderer_ = std::move(renderer);
    
    if (renderer_ && sdl_window_) {
        if (!renderer_->initialize(sdl_window_)) {
            DEARTS_LOG_ERROR("Failed to initialize new renderer");
            renderer_.reset();
        }
    }
}

void Window::setEventHandler(std::shared_ptr<WindowEventHandler> handler) {
    event_handler_ = handler;
}

void Window::handleSDLEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_WINDOWEVENT:
            if (event.window.windowID == SDL_GetWindowID(sdl_window_)) {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:
                        if (event_handler_ && event_handler_->onWindowClose(this)) {
                            close();
                        }
                        break;
                        
                    case SDL_WINDOWEVENT_RESIZED:
                        if (event_handler_) {
                            event_handler_->onWindowResize(this, event.window.data1, event.window.data2);
                        }
                        dispatchEvent(Events::EventType::EVT_WINDOW_RESIZED);
                        break;
                        
                    case SDL_WINDOWEVENT_MOVED:
                        if (event_handler_) {
                            event_handler_->onWindowMove(this, event.window.data1, event.window.data2);
                        }
                        dispatchEvent(Events::EventType::EVT_WINDOW_MOVED);
                        break;
                        
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        if (event_handler_) {
                            event_handler_->onWindowFocusGained(this);
                        }
                        dispatchEvent(Events::EventType::EVT_WINDOW_FOCUS_GAINED);
                        break;
                        
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        if (event_handler_) {
                            event_handler_->onWindowFocusLost(this);
                        }
                        dispatchEvent(Events::EventType::EVT_WINDOW_FOCUS_LOST);
                        break;
                        
                    case SDL_WINDOWEVENT_MINIMIZED:
                        state_ = WindowState::MINIMIZED;
                        if (event_handler_) {
                            event_handler_->onWindowMinimized(this);
                        }
                        dispatchEvent(Events::EventType::EVT_WINDOW_MINIMIZED);
                        break;
                        
                    case SDL_WINDOWEVENT_MAXIMIZED:
                        state_ = WindowState::MAXIMIZED;
                        if (event_handler_) {
                            event_handler_->onWindowMaximized(this);
                        }
                        dispatchEvent(Events::EventType::EVT_WINDOW_MAXIMIZED);
                        break;
                        
                    case SDL_WINDOWEVENT_RESTORED:
                        state_ = WindowState::NORMAL;
                        if (event_handler_) {
                            event_handler_->onWindowRestored(this);
                        }
                        dispatchEvent(Events::EventType::EVT_WINDOW_RESTORED);
                        break;
                        
                    case SDL_WINDOWEVENT_SHOWN:
                        if (event_handler_) {
                            event_handler_->onWindowShown(this);
                        }
                        break;
                        
                    case SDL_WINDOWEVENT_HIDDEN:
                        state_ = WindowState::HIDDEN;
                        if (event_handler_) {
                            event_handler_->onWindowHidden(this);
                        }
                        break;
                        
                    case SDL_WINDOWEVENT_EXPOSED:
                        if (event_handler_) {
                            event_handler_->onWindowExposed(this);
                        }
                        break;
                    default:
                        break;
                }
            }
            break;
    }
}

void Window::updateState() {
    if (!sdl_window_) {
        return;
    }
    
    uint32_t flags = SDL_GetWindowFlags(sdl_window_);
    
    if (flags & SDL_WINDOW_MINIMIZED) {
        state_ = WindowState::MINIMIZED;
    } else if (flags & SDL_WINDOW_MAXIMIZED) {
        state_ = WindowState::MAXIMIZED;
    } else if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
        state_ = WindowState::FULLSCREEN;
    } else if (!(flags & SDL_WINDOW_SHOWN)) {
        state_ = WindowState::HIDDEN;
    } else {
        state_ = WindowState::NORMAL;
    }
}

void Window::dispatchEvent(::DearTs::Core::Events::EventType type) {
    // 这里可以创建具体的窗口事件并分发
    // 目前只是记录日志
    DEARTS_LOG_DEBUG("Window event dispatched: " + std::to_string(static_cast<uint32_t>(type)) + " for window " + std::to_string(id_));
}

// ============================================================================
// WindowManager 实现
// ============================================================================

WindowManager& WindowManager::getInstance() {
    static WindowManager instance;
    return instance;
}

bool WindowManager::initialize() {
    if (initialized_) {
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
    default_config_ = WindowConfig();
    global_vsync_ = true;
    
    initialized_ = true;
    
    DEARTS_LOG_INFO("窗口管理器初始化成功");
    return true;
}

void WindowManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // 销毁所有窗口
    {
        std::lock_guard<std::mutex> lock(windows_mutex_);
        for (auto& [id, window] : windows_) {
            if (window) {
                window->destroy();
            }
        }
        windows_.clear();
    }
    
    // 关闭SDL_image
    IMG_Quit();
    
    // 关闭SDL视频子系统
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    
    initialized_ = false;
    
    DEARTS_LOG_INFO("Window manager shutdown completed");
}

std::shared_ptr<Window> WindowManager::createWindow(const WindowConfig& config) {
    if (!initialized_) {
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
        std::lock_guard<std::mutex> lock(windows_mutex_);
        windows_[window->getId()] = window;
    }
    
    DEARTS_LOG_INFO("Window created: " + config.title + " (ID: " + std::to_string(window->getId()) + ")");
    return window;
}

bool WindowManager::addWindow(std::shared_ptr<Window> window) {
    if (!initialized_) {
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
        std::lock_guard<std::mutex> lock(windows_mutex_);
        windows_[window->getId()] = window;
    }
    
    DEARTS_LOG_INFO("Window added: " + window->getTitle() + " (ID: " + std::to_string(window->getId()) + ")");
    return true;
}

void WindowManager::destroyWindow(uint32_t window_id) {
    DEARTS_LOG_INFO("WindowManager::destroyWindow() called for window ID: " + std::to_string(window_id));
    std::lock_guard<std::mutex> lock(windows_mutex_);
    
    auto it = windows_.find(window_id);
    if (it != windows_.end()) {
        if (it->second) {
            DEARTS_LOG_INFO("Calling destroy() on window ID: " + std::to_string(window_id));
            it->second->destroy();
        }
        windows_.erase(it);
        
        DEARTS_LOG_INFO("Window destroyed: ID " + std::to_string(window_id));
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
    std::lock_guard<std::mutex> lock(windows_mutex_);
    
    auto it = windows_.find(window_id);
    return (it != windows_.end()) ? it->second : nullptr;
}

std::shared_ptr<Window> WindowManager::getWindowBySDLId(uint32_t sdl_window_id) const {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    
    for (const auto& [id, window] : windows_) {
        if (window && window->getSDLWindow()) {
            if (SDL_GetWindowID(window->getSDLWindow()) == sdl_window_id) {
                return window;
            }
        }
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<Window>> WindowManager::getAllWindows() const {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    
    std::vector<std::shared_ptr<Window>> result;
    result.reserve(windows_.size());
    
    for (const auto& [id, window] : windows_) {
        if (window) {
            result.push_back(window);
        }
    }
    
    return result;
}

size_t WindowManager::getWindowCount() const {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    return windows_.size();
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
    DEARTS_LOG_INFO("调用WindowManager::renderAllWindows()");
    auto windows = getAllWindows();
    DEARTS_LOG_INFO("找到 " + std::to_string(windows.size()) + " 个窗口");
    
    // 检查是否有窗口正在拖拽
    bool any_window_dragging = false;
    for (const auto& w : windows) {
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
            DEARTS_LOG_INFO("跳过渲染以降低拖拽时的帧率");
            return;
        }
        
        last_render_time = now;
        DEARTS_LOG_INFO("窗口正在拖拽，降低整体渲染频率");
    } else {
        last_render_time = std::chrono::steady_clock::now();
    }
    
    for (auto& window : windows) {
        if (window && window->isCreated() && window->isVisible()) {
            DEARTS_LOG_INFO("渲染窗口ID: " + std::to_string(window->getId()));
            
            // 检查是否是SDLRenderer并开始ImGui帧
            auto renderer = window->getRenderer();
            
            // 渲染ImGui（如果使用渲染器）
            if (renderer) {
                DEARTS_LOG_INFO("为窗口ID使用渲染器: " + std::to_string(window->getId()));
                
                // 获取SDLRenderer来调用newImGuiFrame和renderImGui方法
                DEARTS_LOG_INFO("检查窗口ID的渲染器是否为SDLRenderer: " + std::to_string(window->getId()));
                auto sdlRenderer = dynamic_cast<DearTs::Core::Render::SDLRenderer*>(renderer);
                DEARTS_LOG_INFO("窗口ID " + std::to_string(window->getId()) + " 的dynamic_cast结果: " + (sdlRenderer ? "成功" : "失败"));
                
                // 检查是否是适配器，如果是则尝试获取底层渲染器
                if (!sdlRenderer) {
                    auto adapter = dynamic_cast<DearTs::Core::Render::IRendererToWindowRendererAdapter*>(renderer);
                    if (adapter) {
                        DEARTS_LOG_INFO("窗口ID的渲染器是适配器: " + std::to_string(window->getId()));
                        // 从适配器中获取底层渲染器
                        auto underlyingRenderer = adapter->getRenderer();
                        if (underlyingRenderer) {
                            sdlRenderer = dynamic_cast<DearTs::Core::Render::SDLRenderer*>(underlyingRenderer.get());
                            DEARTS_LOG_INFO("窗口ID " + std::to_string(window->getId()) + " 的底层渲染器dynamic_cast: " + (sdlRenderer ? "成功" : "失败"));
                        }
                    }
                }
                
                if (sdlRenderer) {
                    DEARTS_LOG_INFO("找到SDLRenderer，为窗口ID启动ImGui帧: " + std::to_string(window->getId()));
                    
                    // 开始ImGui帧
                    sdlRenderer->newImGuiFrame();
                    DEARTS_LOG_INFO("ImGui新帧已启动");
                    
                    // 清除屏幕背景（在ImGui帧开始后清除，避免重影）
                    // 注意：这个清除操作应该在ImGui渲染之前进行，以避免覆盖ImGui内容
                    // 使用颜色值(36,36,36)更接近用户指出的遮挡标题栏的颜色
                    sdlRenderer->clear(36.0f/255.0f, 36.0f/255.0f, 36.0f/255.0f, 1.0f);
                    
                    // 标题栏渲染由具体窗口类处理（如MainWindow）
                    // 这里不再调用window->renderTitleBar()
                    
                    // 在拖拽过程中跳过复杂的内容渲染
                    if (!any_window_dragging) {
                        // 只在非拖拽时渲染复杂内容
                        ImGui::ShowDemoWindow();
                        DEARTS_LOG_INFO("显示ImGui演示窗口");
                    }
                    
                    // 渲染ImGui
                    ImGui::Render();
                    DEARTS_LOG_INFO("调用ImGui::Render()");
                    
                    ImDrawData* draw_data = ImGui::GetDrawData();
                    DEARTS_LOG_INFO("获取ImGui绘制数据，CmdListsCount: " + std::to_string(draw_data ? draw_data->CmdListsCount : -1));
                    
                    sdlRenderer->renderImGui(draw_data);
                    DEARTS_LOG_INFO("使用SDLRenderer渲染ImGui");
                    
                    // 呈现
                    sdlRenderer->present();
                    DEARTS_LOG_INFO("调用SDLRenderer present()");
                } else {
                    // 如果不是SDLRenderer，使用适配器渲染器
                    renderer->beginFrame();
                    
                    // 清空屏幕为深色背景
                    // 使用颜色值(36,36,36)更接近用户指出的遮挡标题栏的颜色
                    renderer->clear(36.0f/255.0f, 36.0f/255.0f, 36.0f/255.0f, 1.0f);
                    
                    // 呈现
                    renderer->endFrame();
                    renderer->present();
                }
            } else {
                DEARTS_LOG_INFO("未找到窗口ID的渲染器: " + std::to_string(window->getId()));
                // 如果没有渲染器，仍然渲染窗口内容
                window->render();
            }
        } else {
            if (!window) {
                DEARTS_LOG_INFO("窗口为空");
            } else if (!window->isCreated()) {
                DEARTS_LOG_INFO("窗口未创建，ID: " + std::to_string(window->getId()));
            } else if (!window->isVisible()) {
                DEARTS_LOG_INFO("窗口不可见，ID: " + std::to_string(window->getId()));
            }
        }
    }
    DEARTS_LOG_INFO("WindowManager::renderAllWindows()完成");
}

void WindowManager::handleSDLEvent(const SDL_Event& event) {
    DEARTS_LOG_INFO("调用WindowManager::handleSDLEvent()，事件类型: " + std::to_string(event.type));
    // 处理窗口事件
    if (event.type == SDL_WINDOWEVENT) {
        auto window = getWindowBySDLId(event.window.windowID);
        if (window) {
            window->handleSDLEvent(event);
        }
    }
    
    // 标题栏事件处理由具体窗口类处理
    // 这里不再调用window->handleTitleBarEvent(event)
}

bool WindowManager::hasWindowsToClose() const {
    auto windows = getAllWindows();
    DEARTS_LOG_DEBUG("Checking hasWindowsToClose, window count: " + std::to_string(windows.size()));
    
    bool result = std::any_of(windows.begin(), windows.end(),
        [](const std::shared_ptr<Window>& window) {
            bool should_close = window && window->shouldClose();
            if (window) {
                DEARTS_LOG_DEBUG("Window ID " + std::to_string(window->getId()) + " shouldClose: " + std::to_string(should_close));
            }
            return should_close;
        });
    
    DEARTS_LOG_DEBUG("hasWindowsToClose result: " + std::to_string(result));
    return result;
}

void WindowManager::closeWindowsToClose() {
    DEARTS_LOG_INFO("WindowManager::closeWindowsToClose() called");
    auto windows = getAllWindows();
    DEARTS_LOG_INFO("Found " + std::to_string(windows.size()) + " windows to check for closing");
    
    int closed_count = 0;
    for (auto& window : windows) {
        if (window && window->shouldClose()) {
            DEARTS_LOG_INFO("Closing window ID: " + std::to_string(window->getId()));
            destroyWindow(window->getId());
            closed_count++;
        }
    }
    
    DEARTS_LOG_INFO("Closed " + std::to_string(closed_count) + " windows");
}

int WindowManager::getDisplayCount() const {
    return SDL_GetNumVideoDisplays();
}

DisplayInfo WindowManager::getDisplayInfo(int display_index) const {
    DisplayInfo info{};
    info.index = display_index;
    
    // 获取显示器名称
    const char* name = SDL_GetDisplayName(display_index);
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

DisplayInfo WindowManager::getPrimaryDisplay() const {
    return getDisplayInfo(0);
}

void WindowManager::setGlobalVSync(bool enabled) {
    global_vsync_ = enabled;
    DEARTS_LOG_INFO("Global VSync set to: " + std::string(enabled ? "enabled" : "disabled"));
}

void WindowManager::setDefaultWindowConfig(const WindowConfig& config) {
    default_config_ = config;
    DEARTS_LOG_DEBUG("Default window config updated");
}

} // namespace Window
} // namespace Core
} // namespace DearTs