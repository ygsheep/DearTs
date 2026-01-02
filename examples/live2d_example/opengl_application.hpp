/**
 * @file opengl_application.hpp
 * @brief OpenGL 应用程序类（Live2D 示例专用）
 * @details 独立的 OpenGL 应用程序，不依赖 DearTs Core Application
 */

#pragma once

#include <SDL3/SDL.h>
// GLEW 必须在 SDL_opengl.h 之前包含
#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <string>
#include <chrono>

namespace Live2DExample {

/**
 * @brief OpenGL 应用程序类
 */
class OpenGLApplication {
public:
    OpenGLApplication();
    ~OpenGLApplication();

    /**
     * @brief 初始化应用程序
     */
    bool initialize();

    /**
     * @brief 运行主循环
     */
    int run();

    /**
     * @brief 关闭应用程序
     */
    void shutdown();

private:
    /**
     * @brief 初始化 SDL
     */
    bool init_sdl();

    /**
     * @brief 创建窗口
     */
    bool create_window();

    /**
     * @brief 初始化 OpenGL
     */
    bool init_opengl();

    /**
     * @brief 初始化 ImGui
     */
    bool init_imgui();

    /**
     * @brief 处理事件
     */
    void process_events();

    /**
     * @brief 更新
     */
    void update();

    /**
     * @brief 渲染
     */
    void render();

    /**
     * @brief 创建 Live2D 界面
     */
    void create_live2d_ui();

private:
    SDL_Window* m_window;
    SDL_GLContext m_gl_context;
    bool m_running;

    // 时间管理
    std::chrono::steady_clock::time_point m_last_frame_time;
    double m_delta_time;

    // 窗口配置
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
    static constexpr const char* WINDOW_TITLE = "Live2D Demo - OpenGL";
};

} // namespace Live2DExample
