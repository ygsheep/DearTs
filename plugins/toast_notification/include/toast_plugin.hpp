/**
 * @file toast_plugin.hpp
 * @brief Toast Notification 插件
 * @details 提供气泡消息通知功能的插件
 */

#pragma once

#include "core/plugin/plugin.h"
#include "core/event/event_bus.h"
#include "core/config/config_manager.h"
#include "core/result.h"
#include "core/tasks/task_events.h"

namespace DearTs::Plugins::Toast {

/**
 * @brief Toast Notification 插件
 *
 * 提供以下功能：
 * - 气泡消息通知系统
 * - 多种消息类型（信息、成功、警告、错误）
 * - 优雅的动画效果
 * - 可配置的显示选项
 * - Toast 测试视图
 */
class ToastPlugin : public Core::Plugin::IPlugin {
public:
    /**
     * @brief 获取插件信息
     */
    Core::Plugin::PluginInfo get_info() const override;

    /**
     * @brief 插件加载时调用
     */
    Core::Result<void, std::string> on_load() override;

    /**
     * @brief 插件卸载时调用
     */
    void on_unload() override;

    /**
     * @brief 插件启用时调用
     */
    void on_enable() override;

    /**
     * @brief 插件禁用时调用
     */
    void on_disable() override;

private:
    /**
     * @brief 注册视图
     */
    void register_views();

    /**
     * @brief 注册命令
     */
    void register_commands();

    /**
     * @brief 加载配置
     */
    void load_config();

    /**
     * @brief 保存配置
     */
    void save_config();

    /**
     * @brief 订阅任务事件
     */
    void subscribe_task_events();

    /**
     * @brief 取消订阅任务事件
     */
    void unsubscribe_task_events();

    /**
     * @brief 处理任务开始事件
     */
    void on_task_started(const Core::Tasks::TaskStartedEvent& event);

    /**
     * @brief 处理任务完成事件
     */
    void on_task_completed(const Core::Tasks::TaskCompletedEvent& event);

    /**
     * @brief 处理任务失败事件
     */
    void on_task_failed(const Core::Tasks::TaskFailedEvent& event);

    /**
     * @brief 处理任务取消事件
     */
    void on_task_cancelled(const Core::Tasks::TaskCancelledEvent& event);

private:
    Core::Event::EventToken m_eventToken;  ///< 事件订阅令牌
    Core::Event::EventToken m_taskStartedToken;    ///< 任务开始事件令牌
    Core::Event::EventToken m_taskCompletedToken;  ///< 任务完成事件令牌
    Core::Event::EventToken m_taskFailedToken;     ///< 任务失败事件令牌
    Core::Event::EventToken m_taskCancelledToken;  ///< 任务取消事件令牌
    Core::Config::ConfigScope m_config{"toast_notification"};  ///< 配置作用域
};

} // namespace DearTs::Plugins::Toast
