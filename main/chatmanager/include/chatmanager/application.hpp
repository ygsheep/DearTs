/**
 * @file application.hpp
 * @brief ChatManager 应用程序类
 */

#pragma once

namespace ChatManager {

/**
 * @brief ChatManager 应用程序
 * @details 这是一个独立的应用程序，只加载 Chat 插件
 */
class Application {
public:
    Application();
    ~Application();

    // 禁止拷贝和移动
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    /**
     * @brief 初始化应用
     * @return 成功返回 true
     */
    bool initialize();

    /**
     * @brief 运行主循环
     */
    void run();

    /**
     * @brief 关闭应用
     */
    void shutdown();

    /**
     * @brief 检查应用是否正在运行
     */
    [[nodiscard]] bool is_running() const { return m_is_running; }

private:
    /**
     * @brief 初始化 DearTs Framework 核心
     */
    bool initialize_core();

    /**
     * @brief 初始化插件系统
     */
    bool initialize_plugin_system();

    /**
     * @brief 初始化窗口
     */
    bool initialize_window();

    /**
     * @brief 主循环
     */
    void main_loop();

    /**
     * @brief 处理事件
     */
    void process_events();

    /**
     * @brief 渲染帧
     */
    void render_frame();

    /**
     * @brief 清理资源
     */
    void cleanup();

    // 状态
    bool m_is_running = false;
    bool m_is_initialized = false;

    // 窗口相关
    void* m_window = nullptr;           // SDL_Window*
    void* m_imgui_context = nullptr;    // ImGuiContext*
};

} // namespace ChatManager
