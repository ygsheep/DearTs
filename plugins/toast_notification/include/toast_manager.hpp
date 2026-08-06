/**
 * @file toast_manager.hpp
 * @brief Toast Notification 管理器
 * @details 管理所有 Toast 消息的创建、更新、销毁和渲染
 */

#pragma once

#include "toast.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace DearTs::Plugins::Toast {

/**
 * @brief Toast Manager - 单例模式
 *
 * 管理所有 Toast 消息的生命周期
 */
class ToastManager final {  // 单例类，禁止继承
public:
    /**
     * @brief 获取单例实例（线程安全，Magic Statics）
     */
    static ToastManager& instance() noexcept {
        static ToastManager s_instance;
        return s_instance;
    }

    // 删除所有拷贝和移动操作
    ToastManager(const ToastManager&) = delete;
    ToastManager& operator=(const ToastManager&) = delete;
    ToastManager(ToastManager&&) = delete;
    ToastManager& operator=(ToastManager&&) = delete;

    /**
     * @brief 显示 Toast 消息
     * @param title 标题
     * @param message 消息内容
     * @param type 消息类型
     * @param duration 显示时长
     * @return Toast ID
     */
    int show(
        const std::string& title,
        const std::string& message,
        ToastType type = ToastType::Info,
        std::chrono::milliseconds duration = std::chrono::milliseconds(3000)
    );

    /**
     * @brief 显示信息提示
     */
    int info(const std::string& title, const std::string& message);

    /**
     * @brief 显示成功提示
     */
    int success(const std::string& title, const std::string& message);

    /**
     * @brief 显示警告提示
     */
    int warning(const std::string& title, const std::string& message);

    /**
     * @brief 显示错误提示
     */
    int error(const std::string& title, const std::string& message);

    /**
     * @brief 关闭指定 Toast
     * @param id Toast ID
     */
    void close(int id);

    /**
     * @brief 关闭所有 Toast
     */
    void close_all();

    /**
     * @brief 更新所有 Toast（每帧调用）
     * @param delta_time 距离上一帧的时间（秒）
     */
    void update(float delta_time);

    /**
     * @brief 渲染所有 Toast（每帧调用）
     */
    void render();

    /**
     * @brief 获取 Toast 数量
     */
    size_t get_count() const {
        return m_toasts.size();
    }

    /**
     * @brief 获取配置
     */
    ToastConfig& get_config() {
        return m_config;
    }

    /**
     * @brief 设置配置
     */
    void set_config(const ToastConfig& config) {
        m_config = config;
    }

    /**
     * @brief 设置配置回调
     */
    using ConfigCallback = std::function<void(ToastConfig&)>;
    void configure(ConfigCallback callback) {
        if (callback) {
            callback(m_config);
        }
    }

private:
    ToastManager() = default;
    ~ToastManager() = default;

    /**
     * @brief 清理已过期的 Toast
     */
    void cleanup_expired();

    /**
     * @brief 移除指定 Toast
     */
    void remove(int id);

    /**
     * @brief 生成唯一 ID
     */
    int generate_id();

    /**
     * @brief 渲染单个 Toast
     */
    void render_toast(ToastMessage& toast, const ImVec2& position);

    /**
     * @brief 绘制带动画的 Toast
     */
    void draw_animated_toast(ToastMessage& toast, const ImVec2& position);

    /**
     * @brief 绘制 Toast 内容
     */
    void draw_toast_content(ToastMessage& toast);

    /**
     * @brief 复制文本到剪贴板
     */
    void copy_to_clipboard(const std::string& text);

    /**
     * @brief 应用缓动函数
     */
    float ease_out_cubic(float x);

    /**
     * @brief 应用缓动函数（弹性效果）
     */
    float ease_out_back(float x);

private:
    std::vector<ToastMessage> m_toasts;    ///< Toast 列表
    int m_next_id = 1;                      ///< 下一个 ID
    ToastConfig m_config;                   ///< 配置
    std::mutex m_mutex;                     ///< 线程安全锁
};

// ================ 便捷函数 ================

/**
 * @brief 显示 Toast 消息（便捷函数）
 */
inline int show_toast(
    const std::string& title,
    const std::string& message,
    ToastType type = ToastType::Info,
    std::chrono::milliseconds duration = std::chrono::milliseconds(3000)
) {
    return ToastManager::instance().show(title, message, type, duration);
}

/**
 * @brief 显示信息提示（便捷函数）
 */
inline int show_info(const std::string& title, const std::string& message) {
    return ToastManager::instance().info(title, message);
}

/**
 * @brief 显示成功提示（便捷函数）
 */
inline int show_success(const std::string& title, const std::string& message) {
    return ToastManager::instance().success(title, message);
}

/**
 * @brief 显示警告提示（便捷函数）
 */
inline int show_warning(const std::string& title, const std::string& message) {
    return ToastManager::instance().warning(title, message);
}

/**
 * @brief 显示错误提示（便捷函数）
 */
inline int show_error(const std::string& title, const std::string& message) {
    return ToastManager::instance().error(title, message);
}

/**
 * @brief 关闭 Toast（便捷函数）
 */
inline void close_toast(int id) {
    ToastManager::instance().close(id);
}

/**
 * @brief 关闭所有 Toast（便捷函数）
 */
inline void close_all_toasts() {
    ToastManager::instance().close_all();
}

} // namespace DearTs::Plugins::Toast
