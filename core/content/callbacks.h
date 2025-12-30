/**
 * @file callbacks.h
 * @brief 回调注册表
 * @details 独立的生命周期回调管理模块
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "registry_base.h"
#include <vector>
#include <functional>

namespace DearTs::Core::ContentRegistry::Callbacks {

/**
 * @brief 回调注册表类
 */
class Registry {
public:
    /**
     * @brief 获取单例实例
     */
    static Registry& instance();

    /**
     * @brief 添加应用初始化完成时的回调
     */
    void add_on_init(Callback callback);

    /**
     * @brief 添加应用关闭前的回调
     */
    void add_on_shutdown(Callback callback);

    /**
     * @brief 添加每帧更新回调
     */
    void add_on_update(std::function<void(double)> callback);

    /**
     * @brief 添加每帧渲染回调
     */
    void add_on_render(Callback callback);

    /**
     * @brief 执行所有初始化回调
     */
    void run_init_callbacks();

    /**
     * @brief 执行所有关闭回调
     */
    void run_shutdown_callbacks();

    /**
     * @brief 执行所有更新回调
     * @param delta_time 时间增量
     */
    void run_update_callbacks(double delta_time);

    /**
     * @brief 执行所有渲染回调
     */
    void run_render_callbacks();

    /**
     * @brief 清空所有回调
     */
    void clear();

private:
    Registry() = default;
    ~Registry() = default;

    // 删除拷贝和移动
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = delete;
    Registry& operator=(Registry&&) = delete;

    std::vector<Callback> m_init_callbacks;
    std::vector<Callback> m_shutdown_callbacks;
    std::vector<std::function<void(double)>> m_update_callbacks;
    std::vector<Callback> m_render_callbacks;
};

/**
 * @brief 便捷函数：添加初始化回调
 */
inline void add_on_init(Callback callback) {
    Registry::instance().add_on_init(std::move(callback));
}

/**
 * @brief 便捷函数：添加关闭回调
 */
inline void add_on_shutdown(Callback callback) {
    Registry::instance().add_on_shutdown(std::move(callback));
}

/**
 * @brief 便捷函数：添加更新回调
 */
inline void add_on_update(std::function<void(double)> callback) {
    Registry::instance().add_on_update(std::move(callback));
}

/**
 * @brief 便捷函数：添加渲染回调
 */
inline void add_on_render(Callback callback) {
    Registry::instance().add_on_render(std::move(callback));
}

/**
 * @brief 便捷函数：执行所有初始化回调
 */
inline void run_init_callbacks() {
    Registry::instance().run_init_callbacks();
}

/**
 * @brief 便捷函数：执行所有关闭回调
 */
inline void run_shutdown_callbacks() {
    Registry::instance().run_shutdown_callbacks();
}

/**
 * @brief 便捷函数：执行所有更新回调
 */
inline void run_update_callbacks(double delta_time) {
    Registry::instance().run_update_callbacks(delta_time);
}

/**
 * @brief 便捷函数：执行所有渲染回调
 */
inline void run_render_callbacks() {
    Registry::instance().run_render_callbacks();
}

} // namespace DearTs::Core::ContentRegistry::Callbacks
