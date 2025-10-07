#pragma once

#include "../../../core/app/application_manager.h"
#include "../../../core/window/window_base.h"
#include "../../../core/window/main_window.h"
#include "../../../core/resource/font_resource.h"
#include "../../../core/resource/resource_manager.h"
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <memory>
#include <string>

namespace DearTs {

/**
 * GUIApplication类 - GUI应用程序类
 * 继承自Application类，提供GUI应用程序功能
 */
class GUIApplication : public Core::App::Application {
public:
    /**
     * 构造函数
     */
    GUIApplication();
    
    /**
     * 析构函数
     */
    ~GUIApplication();
    
    /**
     * 初始化应用程序
     * @param config 应用程序配置
     * @return 是否成功
     */
    bool initialize(const Core::App::ApplicationConfig& config) override;
    
    /**
     * 运行应用程序主循环
     * @return 退出代码
     */
    int run() override;
    
    /**
     * 关闭应用程序
     */
    void shutdown() override;
    
    /**
     * 更新应用程序状态
     * @param delta_time 时间增量（秒）
     */
    void update(double delta_time) override;
    
    /**
     * 渲染应用程序界面
     */
    void render() override;
    
    /**
     * 处理事件
     * @param event 事件
     */
    void handleEvent(const Core::Events::Event& event) override;
    
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

private:
    // 核心组件
    SDL_Window* m_window;           ///< SDL窗口句柄
    SDL_Renderer* m_renderer;       ///< SDL渲染器句柄
    
    // 主窗口
    std::unique_ptr<DearTs::Core::Window::MainWindow> mainWindow_;  ///< 主窗口
    
    /**
     * 初始化SDL
     * @return 初始化是否成功
     */
    bool initializeSDL();
    
    /**
     * 初始化ImGui
     * @return 初始化是否成功
     */
    bool initializeImGui();
    
    /**
     * 处理SDL事件
     */
    void processSDLEvents();
    
    /**
     * 关闭ImGui
     */
    void shutdownImGui();
    
    /**
     * 关闭资源管理器
     */
    void shutdownResourceManager();
    
    /**
     * 关闭SDL
     */
    void shutdownSDL();
};

} // namespace DearTs