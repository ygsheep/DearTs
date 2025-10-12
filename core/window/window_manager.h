/**
 * DearTs Window Manager Header
 * 
 * 窗口管理器头文件 - 提供跨平台窗口管理功能
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <SDL.h>
#include "logger.h"
#include "../events/event_system.h"



namespace DearTs::Core::Window {
    class Window;

// ============================================================================
// 窗口相关枚举和结构
// ============================================================================

/**
 * @brief 窗口状态枚举
 */
enum class WindowState {
    NORMAL,         // 正常状态
    MINIMIZED,      // 最小化
    MAXIMIZED,      // 最大化
    FULLSCREEN,     // 全屏
    HIDDEN,         // 隐藏
    CLOSED          // 已关闭
};

/**
 * @brief 窗口标志
 */
enum class WindowFlags : uint32_t {
    NONE = 0,
    RESIZABLE = SDL_WINDOW_RESIZABLE,
    MINIMIZABLE = SDL_WINDOW_MINIMIZED,
    MAXIMIZABLE = SDL_WINDOW_MAXIMIZED,
    FULLSCREEN = SDL_WINDOW_FULLSCREEN,
    FULLSCREEN_DESKTOP = SDL_WINDOW_FULLSCREEN_DESKTOP,
    BORDERLESS = SDL_WINDOW_BORDERLESS,
    ALWAYS_ON_TOP = SDL_WINDOW_ALWAYS_ON_TOP,
    SKIP_TASKBAR = SDL_WINDOW_SKIP_TASKBAR,
    UTILITY = SDL_WINDOW_UTILITY,
    TOOLTIP = SDL_WINDOW_TOOLTIP,
    POPUP_MENU = SDL_WINDOW_POPUP_MENU,
    KEYBOARD_GRABBED = SDL_WINDOW_KEYBOARD_GRABBED,
    MOUSE_GRABBED = SDL_WINDOW_MOUSE_GRABBED,
    INPUT_GRABBED = SDL_WINDOW_INPUT_GRABBED,
    MOUSE_CAPTURE = SDL_WINDOW_MOUSE_CAPTURE,
    HIGH_DPI = SDL_WINDOW_ALLOW_HIGHDPI,
    VULKAN = SDL_WINDOW_VULKAN,
    METAL = SDL_WINDOW_METAL,
    OPENGL = SDL_WINDOW_OPENGL
};

// 位运算操作符重载
inline WindowFlags operator|(WindowFlags a, WindowFlags b) {
    return static_cast<WindowFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline WindowFlags operator&(WindowFlags a, WindowFlags b) {
    return static_cast<WindowFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline WindowFlags operator^(WindowFlags a, WindowFlags b) {
    return static_cast<WindowFlags>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
}

inline WindowFlags operator~(WindowFlags a) {
    return static_cast<WindowFlags>(~static_cast<uint32_t>(a));
}

/**
 * @brief 窗口位置
 */
struct WindowPosition {
    int x;
    int y;
    
    explicit WindowPosition(int x = SDL_WINDOWPOS_CENTERED, int y = SDL_WINDOWPOS_CENTERED)
        : x(x), y(y) {}
    
    static WindowPosition centered() {
        return WindowPosition(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    
    static WindowPosition undefined() {
        return WindowPosition(SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED);
    }
};

/**
 * @brief 窗口大小
 */
struct WindowSize {
    int width;
    int height;
    
    explicit WindowSize(int w = 800, int h = 600) : width(w), height(h) {}
    
    float aspectRatio() const {
        return height > 0 ? static_cast<float>(width) / height : 0.0f;
    }
};

/**
 * @brief 窗口配置
 */
struct WindowConfig {
    std::string title;                  ///< 窗口标题
    WindowPosition position;            ///< 窗口位置
    WindowSize size;                    ///< 窗口大小
    WindowSize min_size;                ///< 最小大小
    WindowSize max_size;                ///< 最大大小
    WindowFlags flags;                  ///< 窗口标志
    bool vsync;                         ///< 垂直同步
    int display_index;                  ///< 显示器索引
    std::string icon_path;              ///< 图标路径
    
    WindowConfig()
        : title("DearTs Application")
        , position(WindowPosition::centered())
        , size(800, 600)
        , min_size(320, 240)
        , max_size(0, 0)  // 0表示无限制
        , flags(WindowFlags::RESIZABLE)
        , vsync(true)
        , display_index(0)
        , icon_path("resources/icon.ico") {
    }
};

/**
 * @brief 单例窗口配置类
 */
class WindowConfigSingleton {
public:
    /**
     * @brief 获取单例实例
     */
    static WindowConfigSingleton& getInstance() {
        static WindowConfigSingleton instance;
        return instance;
    }
    
    /**
     * @brief 获取窗口配置
     */
    const WindowConfig& getConfig() const {
        return config_;
    }
    
    /**
     * @brief 设置窗口配置
     */
    void setConfig(const WindowConfig& config) {
        config_ = config;
    }
    
    /**
     * @brief 获取窗口标题
     */
    const std::string& getTitle() const {
        return config_.title;
    }
    
    /**
     * @brief 设置窗口标题
     */
    void setTitle(const std::string& title) {
        config_.title = title;
    }
    
    /**
     * @brief 获取窗口位置
     */
    const WindowPosition& getPosition() const {
        return config_.position;
    }
    
    /**
     * @brief 设置窗口位置
     */
    void setPosition(const WindowPosition& position) {
        config_.position = position;
    }
    
    /**
     * @brief 获取窗口大小
     */
    const WindowSize& getSize() const {
        return config_.size;
    }
    
    /**
     * @brief 设置窗口大小
     */
    void setSize(const WindowSize& size) {
        config_.size = size;
    }
    
    /**
     * @brief 获取窗口标志
     */
    WindowFlags getFlags() const {
        return config_.flags;
    }
    
    /**
     * @brief 设置窗口标志
     */
    void setFlags(WindowFlags flags) {
        config_.flags = flags;
    }
    
private:
    WindowConfigSingleton() = default;
    ~WindowConfigSingleton() = default;
    WindowConfigSingleton(const WindowConfigSingleton&) = delete;
    WindowConfigSingleton& operator=(const WindowConfigSingleton&) = delete;
    
    WindowConfig config_;
};

/**
 * @brief 显示器信息
 */
struct DisplayInfo {
    int index;                          ///< 显示器索引
    std::string name;                   ///< 显示器名称
    SDL_Rect bounds;                    ///< 显示器边界
    SDL_Rect usable_bounds;             ///< 可用区域
    float dpi;                          ///< DPI
    int refresh_rate;                   ///< 刷新率
    SDL_PixelFormat* pixel_format;      ///< 像素格式
    bool is_primary;                    ///< 是否为主显示器
};

// ============================================================================
// 窗口事件处理器
// ============================================================================

/**
 * @brief 窗口事件处理器接口
 */
class WindowEventHandler {
public:
    virtual ~WindowEventHandler() = default;
    
    /**
     * @brief 窗口关闭事件
     */
    virtual bool onWindowClose(::DearTs::Core::Window::Window* window) {
        return true;
    }
    
    /**
     * @brief 窗口大小改变事件
     */
    virtual void onWindowResize(::DearTs::Core::Window::Window* window, int width, int height) {}
    
    /**
     * @brief 窗口移动事件
     */
    virtual void onWindowMove(::DearTs::Core::Window::Window* window, int x, int y) {}
    
    /**
     * @brief 窗口获得焦点事件
     */
    virtual void onWindowFocusGained(Window* window) {}
    
    /**
     * @brief 窗口失去焦点事件
     */
    virtual void onWindowFocusLost(Window* window) {}
    
    /**
     * @brief 窗口最小化事件
     */
    virtual void onWindowMinimized(Window* window) {}
    
    /**
     * @brief 窗口最大化事件
     */
    virtual void onWindowMaximized(Window* window) {}
    
    /**
     * @brief 窗口恢复事件
     */
    virtual void onWindowRestored(Window* window) {}
    
    /**
     * @brief 窗口显示事件
     */
    virtual void onWindowShown(Window* window) {}
    
    /**
     * @brief 窗口隐藏事件
     */
    virtual void onWindowHidden(Window* window) {}
    
    /**
     * @brief 窗口暴露事件（需要重绘）
     */
    virtual void onWindowExposed(Window* window) {}
};

// ============================================================================
// 窗口渲染器接口
// ============================================================================

/**
 * @brief 窗口渲染器接口
 */
class WindowRenderer {
public:
    virtual ~WindowRenderer() = default;
    
    /**
     * @brief 初始化渲染器
     */
    virtual bool initialize(SDL_Window* window) = 0;
    
    /**
     * @brief 关闭渲染器
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief 开始渲染帧
     */
    virtual void beginFrame() = 0;
    
    /**
     * @brief 结束渲染帧
     */
    virtual void endFrame() = 0;
    
    /**
     * @brief 呈现渲染结果
     */
    virtual void present() = 0;
    
    /**
     * @brief 清空渲染目标
     */
    virtual void clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) = 0;
    
    /**
     * @brief 设置视口
     */
    virtual void setViewport(int x, int y, int width, int height) = 0;
    
    /**
     * @brief 获取渲染器类型
     */
    virtual std::string getType() const = 0;
    
    /**
     * @brief 检查是否已初始化
     */
    virtual bool isInitialized() const = 0;
};

// ============================================================================
// 窗口类
// ============================================================================

/**
 * @brief 窗口类
 */
class Window : public std::enable_shared_from_this<Window> {
public:
    /**
     * @brief 构造函数
     */
    explicit Window(const WindowConfig& config);
    
    /**
     * @brief 析构函数
     */
    ~Window();
    
    /**
     * @brief 创建窗口
     */
    bool create();
    
    /**
     * @brief 销毁窗口
     */
    void destroy();
    
    /**
     * @brief 显示窗口
     */
    void show();
    
    /**
     * @brief 隐藏窗口
     */
    void hide();
    
    /**
     * @brief 最小化窗口
     */
    void minimize();
    
    /**
     * @brief 最大化窗口
     */
    void maximize();
    
    /**
     * @brief 恢复窗口
     */
    void restore();
    
    /**
     * @brief 设置全屏模式
     */
    void setFullscreen(bool fullscreen, bool desktop_fullscreen = false);
    
    /**
     * @brief 关闭窗口
     */
    void close();
    
    /**
     * @brief 更新窗口
     */
    void update();
    
    /**
     * @brief 渲染窗口
     */
    void render();
    
    // ========================================================================
    // 标题栏管理
    // ========================================================================
    
    
    
    // ========================================================================
    // 属性访问器
    // ========================================================================
    
    /**
     * @brief 获取窗口ID
     */
    uint32_t getId() const { return m_id; }

    /**
     * @brief 获取SDL窗口句柄
     */
    SDL_Window* getSDLWindow() const { return m_sdlWindow; }
    
    /**
     * @brief 获取窗口标题
     */
    std::string getTitle() const;
    
    /**
     * @brief 设置窗口标题
     */
    void setTitle(const std::string& title);
    
    /**
     * @brief 获取窗口位置
     */
    WindowPosition getPosition() const;
    
    /**
     * @brief 设置窗口位置
     */
    void setPosition(const WindowPosition& position);
    
    /**
     * @brief 获取窗口大小
     */
    WindowSize getSize() const;
    
    /**
     * @brief 设置窗口大小
     */
    void setSize(const WindowSize& size);
    
    /**
     * @brief 获取窗口最小大小
     */
    WindowSize getMinSize() const;
    
    /**
     * @brief 设置窗口最小大小
     */
    void setMinSize(const WindowSize& size);
    
    /**
     * @brief 获取窗口最大大小
     */
    WindowSize getMaxSize() const;
    
    /**
     * @brief 设置窗口最大大小
     */
    void setMaxSize(const WindowSize& size);
    
    /**
     * @brief 获取窗口状态
     */
    WindowState getState() const { return m_state; }
    
    /**
     * @brief 获取窗口标志
     */
    WindowFlags getFlags() const;
    
    /**
     * @brief 设置窗口图标
     */
    bool setIcon(const std::string& icon_path);
    
    /**
     * @brief 获取窗口透明度
     */
    float getOpacity() const;
    
    /**
     * @brief 设置窗口透明度
     */
    void setOpacity(float opacity) const;
    
    /**
     * @brief 检查窗口是否可见
     */
    bool isVisible() const;
    
    /**
     * @brief 检查窗口是否有焦点
     */
    bool hasFocus() const;
    
    /**
     * @brief 检查窗口是否已创建
     */
    bool isCreated() const { return m_sdlWindow != nullptr; }
    
    /**
     * @brief 检查窗口是否应该关闭
     */
    bool shouldClose() const { return m_shouldClose; }
    
    /**
     * @brief 获取显示器索引
     */
    int getDisplayIndex() const;
    
    /**
     * @brief 获取DPI缩放因子
     */
    float getDPIScale() const;
    
    // ========================================================================
    // 渲染器管理
    // ========================================================================
    
    /**
     * @brief 设置渲染器
     */
    void setRenderer(std::unique_ptr<WindowRenderer> renderer);
    
    /**
     * @brief 获取渲染器
     */
    WindowRenderer* getRenderer() const { return m_renderer.get(); }
    
    // ========================================================================
    // 事件处理
    // ========================================================================
    
    /**
     * @brief 设置事件处理器
     */
    void setEventHandler(std::shared_ptr<WindowEventHandler> handler);
    
    /**
     * @brief 获取事件处理器
     */
    std::shared_ptr<WindowEventHandler> getEventHandler() const { return m_eventHandler; }
    
    /**
     * @brief 处理SDL事件
     */
    void handleSDLEvent(const SDL_Event& event);
    
    // ========================================================================
    // 用户数据
    // ========================================================================
    
    /**
     * @brief 设置用户数据
     */
    void setUserData(void* data) { m_userData = data; }
    
    /**
     * @brief 获取用户数据
     */
    void* getUserData() const { return m_userData; }

    /**
     * @brief 设置拖拽状态
     */
    void setDragging(bool dragging) { m_isDragging = dragging; }

    /**
     * @brief 检查是否正在拖拽
     */
    bool isDragging() const { return m_isDragging; }
    
private:
    /**
     * @brief 更新窗口状态
     */
    void updateState();

    /**
     * @brief 分发窗口事件
     */
    void dispatchEvent(DearTs::Core::Events::EventType type);

    uint32_t m_id;                                          ///< 窗口ID
    WindowConfig m_config;                                  ///< 窗口配置
    SDL_Window* m_sdlWindow;                                ///< SDL窗口句柄
    WindowState m_state;                                    ///< 窗口状态
    bool m_shouldClose;                                     ///< 是否应该关闭
    bool m_isDragging;                                      ///< 是否正在拖拽窗口
    void* m_userData;                                       ///< 用户数据

    std::unique_ptr<WindowRenderer> m_renderer;             ///< 渲染器
    std::shared_ptr<WindowEventHandler> m_eventHandler;      ///< 事件处理器

    static std::atomic<uint32_t> s_nextId;                  ///< 下一个窗口ID
};

} // namespace DearTs::Core::Window

// ============================================================================
// 窗口管理器
// ============================================================================

namespace DearTs::Core::Window {

/**
 * @brief 窗口管理器(单例)
 */
class WindowManager {
public:
    /**
     * @brief 获取单例实例
     */
    static WindowManager& getInstance();
    
    /**
     * @brief 初始化窗口管理器
     */
    bool initialize();
    
    /**
     * @brief 关闭窗口管理器
     */
    void shutdown();
    
    /**
     * @brief 创建窗口
     */
    std::shared_ptr<Window> createWindow(const WindowConfig& config);
    
    /**
     * @brief 添加已创建的窗口
     */
    bool addWindow(std::shared_ptr<Window> window);

    /**
     * @brief 添加已创建的窗口并指定名称
     */
    bool addWindow(const std::string& name, std::shared_ptr<Window> window);

    /**
     * @brief 销毁窗口
     */
    void destroyWindow(uint32_t window_id);
    
    /**
     * @brief 销毁窗口
     */
    void destroyWindow(std::shared_ptr<Window> window);
    
    /**
     * @brief 获取窗口
     */
    std::shared_ptr<Window> getWindow(uint32_t window_id) const;
    
    /**
     * @brief 根据SDL窗口ID获取窗口
     */
    std::shared_ptr<Window> getWindowBySDLId(uint32_t sdl_window_id) const;

    /**
     * @brief 根据名称获取窗口
     */
    std::shared_ptr<Window> getWindowByName(const std::string& name) const;

    /**
     * @brief 获取所有窗口
     */
    std::vector<std::shared_ptr<Window>> getAllWindows() const;
    
    /**
     * @brief 获取窗口数量
     */
    size_t getWindowCount() const;
    
    /**
     * @brief 更新所有窗口
     */
    void updateAllWindows();
    
    /**
     * @brief 渲染所有窗口
     */
    void renderAllWindows();
    
    /**
     * @brief 处理SDL事件
     */
    void handleSDLEvent(const SDL_Event& event);
    
    /**
     * @brief 检查是否有窗口应该关闭
     */
    bool hasWindowsToClose() const;
    
    /**
     * @brief 关闭所有应该关闭的窗口
     */
    void closeWindowsToClose();

    // ========================================================================
    // 按名称管理窗口
    // ========================================================================

    /**
     * @brief 根据名称显示窗口
     */
    bool showWindow(const std::string& name);

    /**
     * @brief 根据名称隐藏窗口
     */
    bool hideWindow(const std::string& name);

    /**
     * @brief 根据名称切换窗口显示状态
     */
    bool toggleWindow(const std::string& name);

    /**
     * @brief 检查指定名称的窗口是否可见
     */
    bool isWindowVisible(const std::string& name) const;

    /**
     * @brief 根据名称设置窗口焦点
     */
    bool focusWindow(const std::string& name);

    // ========================================================================
    // 显示器管理
    // ========================================================================
    
    /**
     * @brief 获取显示器数量
     */
    int getDisplayCount() const;
    
    /**
     * @brief 获取显示器信息
     */
    DisplayInfo getDisplayInfo(int display_index) const;
    
    /**
     * @brief 获取所有显示器信息
     */
    std::vector<DisplayInfo> getAllDisplays() const;
    
    /**
     * @brief 获取主显示器信息
     */
    DisplayInfo getPrimaryDisplay() const;
    
    // ========================================================================
    // 全局设置
    // ========================================================================
    
    /**
     * @brief 设置全局垂直同步
     */
    void setGlobalVSync(bool enabled);
    
    /**
     * @brief 获取全局垂直同步状态
     */
    bool getGlobalVSync() const { return m_globalVSync; }

    /**
     * @brief 设置默认窗口配置
     */
    void setDefaultWindowConfig(const WindowConfig& config);

    /**
     * @brief 获取默认窗口配置
     */
    const WindowConfig& getDefaultWindowConfig() const { return m_defaultConfig; }

    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const { return m_initialized; }
    
private:
    WindowManager()
        : m_globalVSync(true) {
    }
    ~WindowManager() = default;
    
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
    
    std::unordered_map<uint32_t, std::shared_ptr<Window>> m_windows;     ///< 窗口映射（按ID）
    std::unordered_map<std::string, std::shared_ptr<Window>> m_namedWindows; ///< 窗口映射（按名称）
    mutable std::mutex m_windowsMutex;                                  ///< 窗口互斥锁

    WindowConfig m_defaultConfig;                                       ///< 默认窗口配置
    bool m_globalVSync;                                                 ///< 全局垂直同步
    std::atomic<bool> m_initialized{false};                            ///< 初始化标志
};

} // namespace DearTs::Core::Window

// ============================================================================
// 便利宏定义
// ============================================================================
// 便利宏
#define DEARTS_WINDOW_MANAGER() DearTs::Core::Window::WindowManager::getInstance()
#define DEARTS_CREATE_WINDOW(config) DEARTS_WINDOW_MANAGER().createWindow(config)
#define DEARTS_GET_WINDOW(id) DEARTS_WINDOW_MANAGER().getWindow(id)
#define DEARTS_DESTROY_WINDOW(id) DEARTS_WINDOW_MANAGER().destroyWindow(id)

