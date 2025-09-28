#pragma once

#include "window_manager.h"
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <memory>
#include <string>
#include "../../../lib/libdearts/include/dearts/api/event_manager.hpp"

namespace DearTs::GUI {

// 引入EventManager到当前命名空间
using dearts::EventManager;

// 定义应用程序相关事件
EVENT_DEF(ApplicationInitializedEvent, void*);
EVENT_DEF(ApplicationShutdownEvent, void*);
EVENT_DEF(ApplicationExitRequestedEvent, void*);
EVENT_DEF(ApplicationUpdateEvent, void*);

/**
 * ApplicationManager类 - 应用程序主管理器
 * 负责整个应用程序的生命周期管理，集成所有子系统
 * 基于ImHex的架构设计，提供现代化的应用程序框架
 */
class ApplicationManager {
public:
    /**
     * 构造函数
     */
    ApplicationManager();
    
    /**
     * 析构函数
     */
    ~ApplicationManager();
    
    /**
     * 初始化应用程序
     * 按顺序初始化所有子系统
     * @return 初始化是否成功
     */
    bool initialize();
    
    /**
     * 运行主循环
     * 处理事件、更新状态、渲染界面
     */
    void run();
    
    /**
     * 关闭应用程序
     * 清理所有资源
     */
    void shutdown();
    
    /**
     * 请求退出应用程序
     */
    void requestExit();
    
    /**
     * 检查应用程序是否正在运行
     * @return 是否正在运行
     */
    bool isRunning() const { return m_running; }
    
    /**
     * 检查应用程序是否已初始化
     * @return 是否已初始化
     */
    bool isInitialized() const { return m_initialized; }
    
    /**
     * 获取SDL窗口句柄
     * @return SDL窗口句柄
     */
    SDL_Window* getWindow() const { return m_window; }
    
    /**
     * 获取SDL渲染器句柄
     * @return SDL渲染器句柄
     */
    SDL_Renderer* getRenderer() const { return m_renderer; }
    
    /**
     * 获取窗口管理器
     * @return 窗口管理器指针
     */
    WindowManager* getWindowManager() const { return m_windowManager; }
    
    // 禁用拷贝构造和赋值操作
    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;
    
    // 禁用移动构造和赋值操作
    ApplicationManager(ApplicationManager&&) = delete;
    ApplicationManager& operator=(ApplicationManager&&) = delete;

private:
    // 核心组件
    SDL_Window* m_window;           ///< SDL窗口句柄
    SDL_Renderer* m_renderer;       ///< SDL渲染器句柄
    WindowManager* m_windowManager; ///< 窗口管理器
    
    // 状态标志
    bool m_running;                 ///< 是否正在运行
    bool m_initialized;             ///< 是否已初始化
    
    /**
     * 初始化SDL
     * @return 初始化是否成功
     */
    bool initializeSDL();
    
    /**
     * 初始化ResourceManager
     * @return 初始化是否成功
     */
    bool initializeResourceManager();
    
    /**
     * 初始化ImGui
     * @return 初始化是否成功
     */
    bool initializeImGui();
    
    /**
     * 初始化窗口管理器
     * @return 初始化是否成功
     */
    bool initializeWindowManager();
    
    /**
     * 初始化主题系统
     * @return 初始化是否成功
     */
    bool initializeTheme();
    
    /**
     * 初始化字体系统
     * @return 初始化是否成功
     */
    static bool initializeFonts();
    
    /**
     * 初始化插件系统
     * @return 初始化是否成功
     */
    bool initializePlugins();
    
    /**
     * 处理事件
     */
    void processEvents();
    
    /**
     * 更新状态
     */
    void update();
    
    /**
     * 渲染界面
     */
    void render();
    
    /**
     * 渲染主界面
     */
    void renderMainInterface();
    
    /**
     * 注册事件处理器
     */
    void registerEventHandlers();
    
    /**
     * 取消事件处理器注册
     */
    void unregisterEventHandlers();
    
    /**
     * 初始化日志系统
     */
    void initializeLogging();
    
    /**
     * 关闭ImGui
     */
    void shutdownImGui();
    
    /**
     * 关闭SDL
     */
    void shutdownSDL();
    
    /**
     * 关闭插件系统
     */
    void shutdownPlugins();
};

} // namespace DearTs::GUI