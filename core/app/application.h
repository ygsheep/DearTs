/**
 * @file application.h
 * @brief 应用程序基类
 * @details 提供应用程序生命周期管理，遵循 DearTs 代码规范
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "logger.h"
#include <SDL3/SDL.h>
#include <string>
#include <atomic>
#include <chrono>
#include <functional>

namespace DearTs::Core::App {

    /**
 * @brief 应用程序状态枚举
 */
    enum class ApplicationState {
        UNINITIALIZED,  ///< 未初始化
        INITIALIZING,   ///< 初始化中
        RUNNING,        ///< 运行中
        PAUSED,         ///< 已暂停
        STOPPING,       ///< 停止中
        STOPPED         ///< 已停止
    };

    /**
 * @brief 应用程序配置结构
 */
    struct ApplicationConfig {
        std::string name = "DearTs Application";  ///< 应用程序名称
        std::string version = "1.0.0";              ///< 版本号
        int window_width = 1280;                    ///< 窗口宽度
        int window_height = 720;                    ///< 窗口高度
        bool enable_vsync = true;                   ///< 垂直同步
        bool enable_imgui = true;                   ///< 启用 ImGui
        bool borderless = false;                    ///< 无边框窗口（用于自定义标题栏）
        bool transparent = false;                   ///< 透明窗口（SDL_PROP_WINDOW_CREATE_TRANSPARENT_BOOLEAN）
        bool always_on_top = false;                 ///< 置顶窗口
    };

    /**
 * @brief 应用程序基类
 * @details 管理应用程序的完整生命周期：初始化 -> 运行 -> 关闭
 */
    class Application {
    public:
        /**
     * @brief 构造函数
     */
        Application();

        /**
     * @brief 虚析构函数
     */
        virtual ~Application();

        /**
     * @brief 初始化应用程序
     * @param config 应用程序配置
     * @return 成功返回 true，失败返回 false
     */
        virtual bool initialize(const ApplicationConfig& config);

        /**
     * @brief 运行应用程序主循环
     * @return 退出代码
     */
        virtual int run();

        /**
     * @brief 关闭应用程序
     */
        virtual void shutdown();

        /**
     * @brief 请求退出应用程序
     * @param exit_code 退出代码，默认为 0
     */
        void request_exit(int exit_code = 0);

        /**
     * @brief 获取应用程序状态
     * @return 当前应用程序状态
     */
        ApplicationState get_state() const { return m_state; }

        /**
     * @brief 获取应用程序配置
     * @return 应用程序配置的常量引用
     */
        const ApplicationConfig& get_config() const { return m_config; }

        /**
     * @brief 获取退出代码
     * @return 退出代码
     */
        int get_exit_code() const { return m_exit_code.load(); }

        /**
     * @brief 检查是否应该退出
     * @return 如果应该退出返回 true
     */
        bool should_exit() const { return m_should_exit.load(); }

    protected:
        /**
     * @brief 初始化回调函数
     * @details 子类重写此方法以执行自定义初始化逻辑
     * @return 成功返回 true，失败返回 false
     */
        virtual bool on_init() { return true; }

        /**
     * @brief 更新回调函数
     * @details 子类重写此方法以执行每帧更新逻辑
     * @param delta_time 时间增量（秒）
     */
        virtual void on_update(double delta_time) {
            (void)delta_time; // 消除未使用参数警告
        }

        /**
     * @brief 渲染回调函数
     * @details 子类重写此方法以执行渲染逻辑
     */
        virtual void on_render() {}

        /**
     * @brief 事件处理回调函数
     * @details 子类重写此方法以处理 SDL 事件
     * @param event SDL 事件
     */
        virtual void on_event(const SDL_Event& event) {
            (void)event; // 消除未使用参数警告
        }

        /**
     * @brief 关闭回调函数
     * @details 子类重写此方法以执行自定义清理逻辑
     */
        virtual void on_shutdown() {}

        /**
     * @brief 暂停回调函数
     * @details 子类重写此方法以处理暂停事件
     */
        virtual void on_pause() {}

        /**
     * @brief 恢复回调函数
     * @details 子类重写此方法以处理恢复事件
     */
        virtual void on_resume() {}

    protected:
        /**
     * @brief 处理 SDL 事件
     */
        void process_events();

        /**
     * @brief 限制帧率
     */
        void limit_frame_rate();

        /**
     * @brief 更新应用程序统计信息
     */
        void update_stats();

    protected:
        ApplicationConfig m_config;                           ///< 应用程序配置
        ApplicationState m_state;                             ///< 应用程序状态
        std::atomic<int> m_exit_code;                         ///< 退出代码
        std::atomic<bool> m_should_exit;                      ///< 是否应该退出

        // 时间管理
        std::chrono::steady_clock::time_point m_last_frame_time;  ///< 上一帧时间
        std::chrono::steady_clock::time_point m_fps_timer;         ///< FPS 计时器
        double m_delta_time;                                      ///< 时间增量（秒）
        uint32_t m_frame_count;                                   ///< 帧计数
        uint32_t m_fps_frame_count;                               ///< FPS 帧计数
        double m_current_fps;                                     ///< 当前 FPS
        double m_average_fps;                                     ///< 平均 FPS

        // SDL 相关
        SDL_Window* m_window;                                 ///< SDL 窗口
        SDL_Renderer* m_renderer;                             ///< SDL 渲染器

        // 性能配置
        bool m_enable_vsync;                                  ///< 是否启用垂直同步
        uint32_t m_target_fps;                                ///< 目标帧率
        double m_frame_time;                                  ///< 每帧目标时间（秒）
    };

}
