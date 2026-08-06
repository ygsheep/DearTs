/**
 * @file toast_plugin.cpp
 * @brief Toast Plugin 实现
 */

#include "toast_plugin.hpp"
#include "toast_manager.hpp"
#include "toast_view.hpp"
#include "core/content/commands.h"
#include "core/tasks/task_manager.h"
#include "liblogger/logger.h"
#include <imgui.h>
#include <format>
#include <algorithm>
#include <vector>

namespace DearTs::Plugins::Toast {

// ================ ToastPlugin 实现 ================

Core::Plugin::PluginInfo ToastPlugin::get_info() const {
    return Core::Plugin::PluginInfo{
        .name = "Toast Notification",
        .author = "DearTs Team",
        .description = "气泡消息通知系统，提供优雅的 UI 提示",
        .version = "1.0.0",
        .api_version = "1.0.0"
    };
}

Core::Result<void, std::string> ToastPlugin::on_load() {
    LOG_INFO("ToastPlugin: Loading...");

    try {
        // 加载配置
        load_config();

        // 注册视图
        register_views();

        // 注册命令
        register_commands();

        // 订阅任务事件
        subscribe_task_events();

        LOG_INFO("ToastPlugin: Loaded successfully");
        return Core::Result<void, std::string>::ok();

    } catch (const std::exception& e) {
        std::string error = std::string("Failed to load ToastPlugin: ") + e.what();
        LOG_ERROR("{}", error);
        return Core::Result<void, std::string>::err(error);
    }
}

void ToastPlugin::on_unload() {
    LOG_INFO("ToastPlugin: Unloading...");

    // 取消订阅任务事件
    unsubscribe_task_events();

    // 保存配置
    save_config();

    // 清理资源
    ToastManager::instance().close_all();

    LOG_INFO("ToastPlugin: Unloaded");
}

void ToastPlugin::on_enable() {
    LOG_INFO("ToastPlugin: Enabled");

    // 显示欢迎消息
    // ToastManager::instance().success(
    //     "Toast Plugin 已启用",
    //     "气泡消息通知系统已准备就绪"
    // );
}

void ToastPlugin::on_disable() {
    LOG_INFO("ToastPlugin: Disabled");

    // 关闭所有 Toast
    ToastManager::instance().close_all();
}

void ToastPlugin::register_views() {
    LOG_DEBUG("ToastPlugin: Registering views...");

    // 注册 Toast 测试视图
    Core::ContentRegistry::Views::add<ToastView>();

    LOG_DEBUG("ToastPlugin: Views registered");
}

void ToastPlugin::register_commands() {
    LOG_DEBUG("ToastPlugin: Registering commands...");

    // 显示信息 Toast
    Core::ContentRegistry::Commands::add(
        "toast.info",
        "显示信息提示",
        []() {
            ToastManager::instance().info(
                "信息",
                "这是一条信息提示"
            );
        }
    );

    // 显示成功 Toast
    Core::ContentRegistry::Commands::add(
        "toast.success",
        "显示成功提示",
        []() {
            ToastManager::instance().success(
                "成功",
                "操作已成功完成"
            );
        }
    );

    // 显示警告 Toast
    Core::ContentRegistry::Commands::add(
        "toast.warning",
        "显示警告提示",
        []() {
            ToastManager::instance().warning(
                "警告",
                "请注意可能存在的问题"
            );
        }
    );

    // 显示错误 Toast
    Core::ContentRegistry::Commands::add(
        "toast.error",
        "显示错误提示",
        []() {
            ToastManager::instance().error(
                "错误",
                "操作失败，请重试"
            );
        }
    );

    // 关闭所有 Toast
    Core::ContentRegistry::Commands::add(
        "toast.close_all",
        "关闭所有提示",
        []() {
            ToastManager::instance().close_all();
        }
    );

    LOG_DEBUG("ToastPlugin: Commands registered");
}

void ToastPlugin::load_config() {
    LOG_DEBUG("ToastPlugin: Loading configuration...");

    auto& config = ToastManager::instance().get_config();

    // 从配置文件加载设置（使用新的默认值）
    config.animation_speed = m_config.get_or<double>("animation_speed", 3.0);
    config.enter_duration = m_config.get_or<double>("enter_duration", 0.5);  // 增加到 0.5s
    config.exit_duration = m_config.get_or<double>("exit_duration", 0.3);    // 增加到 0.3s
    config.max_width = m_config.get_or<double>("max_width", 400.0);
    config.min_width = m_config.get_or<double>("min_width", 300.0);
    config.padding_x = m_config.get_or<double>("padding_x", 20.0);
    config.padding_y = m_config.get_or<double>("padding_y", 16.0);
    config.spacing = m_config.get_or<double>("spacing", 8.0);
    config.max_toasts = m_config.get_or<int>("max_toasts", 5);
    config.position = m_config.get_or<int>("position", static_cast<int>(ToastPosition::TopRight));
    config.show_progress_bar = m_config.get_or<bool>("show_progress_bar", true);
    config.show_close_button = m_config.get_or<bool>("show_close_button", true);
    config.show_copy_button = m_config.get_or<bool>("show_copy_button", true);
    config.pause_on_hover = m_config.get_or<bool>("pause_on_hover", true);
    config.click_to_close = m_config.get_or<bool>("click_to_close", false);

    // 安全检查：确保所有配置值有效
    if (config.max_width <= 0.0) {
        LOG_WARN("ToastPlugin: Invalid max_width ({:.1f}), using default value", config.max_width);
        config.max_width = 400.0;
        m_config.set("max_width", 400.0);
    }
    if (config.min_width <= 0.0) {
        LOG_WARN("ToastPlugin: Invalid min_width ({:.1f}), using default value", config.min_width);
        config.min_width = 300.0;
        m_config.set("min_width", 300.0);
    }
    if (config.padding_x < 16.0) {
        LOG_WARN("ToastPlugin: padding_x too small ({:.1f}), updating to 20.0", config.padding_x);
        config.padding_x = 20.0;
        m_config.set("padding_x", 20.0);
    }
    if (config.padding_y < 12.0) {
        LOG_WARN("ToastPlugin: padding_y too small ({:.1f}), updating to 16.0", config.padding_y);
        config.padding_y = 16.0;
        m_config.set("padding_y", 16.0);
    }
    // 动画时长验证（关键！）
    if (config.enter_duration <= 0.0) {
        LOG_WARN("ToastPlugin: Invalid enter_duration ({:.6f}), using default value 0.5", config.enter_duration);
        config.enter_duration = 0.5;
        m_config.set("enter_duration", 0.5);
    }
    if (config.exit_duration <= 0.0) {
        LOG_WARN("ToastPlugin: Invalid exit_duration ({:.6f}), using default value 0.3", config.exit_duration);
        config.exit_duration = 0.3;
        m_config.set("exit_duration", 0.3);
    }
    if (config.position < 0 || config.position > 5) {
        LOG_WARN("ToastPlugin: Invalid position ({}), using default value TopRight", config.position);
        config.position = static_cast<int>(ToastPosition::TopRight);
        m_config.set("position", static_cast<int>(ToastPosition::TopRight));
    }

    LOG_DEBUG("ToastPlugin: Configuration loaded");
}

void ToastPlugin::save_config() {
    LOG_DEBUG("ToastPlugin: Saving configuration...");

    auto& config = ToastManager::instance().get_config();

    // 保存设置到配置文件
    m_config.set("animation_speed", config.animation_speed);
    m_config.set("enter_duration", config.enter_duration);
    m_config.set("exit_duration", config.exit_duration);
    m_config.set("max_width", config.max_width);
    m_config.set("min_width", config.min_width);
    m_config.set("padding_x", config.padding_x);
    m_config.set("padding_y", config.padding_y);
    m_config.set("spacing", config.spacing);
    m_config.set("max_toasts", config.max_toasts);
    m_config.set("position", config.position);
    m_config.set("show_progress_bar", config.show_progress_bar);
    m_config.set("show_close_button", config.show_close_button);
    m_config.set("pause_on_hover", config.pause_on_hover);
    m_config.set("click_to_close", config.click_to_close);

    LOG_DEBUG("ToastPlugin: Configuration saved");
}

void ToastPlugin::subscribe_task_events() {
    LOG_DEBUG("ToastPlugin: Subscribing to task events...");

    auto& event_bus = Core::Event::EventBus::instance();

    // 订阅任务开始事件
    m_taskStartedToken = event_bus.subscribe<Core::Tasks::TaskStartedEvent>(
        [this](const Core::Tasks::TaskStartedEvent& event) {
            this->on_task_started(event);
        }
    );

    // 订阅任务完成事件
    m_taskCompletedToken = event_bus.subscribe<Core::Tasks::TaskCompletedEvent>(
        [this](const Core::Tasks::TaskCompletedEvent& event) {
            this->on_task_completed(event);
        }
    );

    // 订阅任务失败事件
    m_taskFailedToken = event_bus.subscribe<Core::Tasks::TaskFailedEvent>(
        [this](const Core::Tasks::TaskFailedEvent& event) {
            this->on_task_failed(event);
        }
    );

    // 订阅任务取消事件
    m_taskCancelledToken = event_bus.subscribe<Core::Tasks::TaskCancelledEvent>(
        [this](const Core::Tasks::TaskCancelledEvent& event) {
            this->on_task_cancelled(event);
        }
    );

    LOG_DEBUG("ToastPlugin: Task events subscribed");
}

void ToastPlugin::unsubscribe_task_events() {
    LOG_DEBUG("ToastPlugin: Unsubscribing from task events...");

    auto& event_bus = Core::Event::EventBus::instance();

    // 取消订阅所有任务事件
    event_bus.unsubscribe<Core::Tasks::TaskStartedEvent>(m_taskStartedToken);
    event_bus.unsubscribe<Core::Tasks::TaskCompletedEvent>(m_taskCompletedToken);
    event_bus.unsubscribe<Core::Tasks::TaskFailedEvent>(m_taskFailedToken);
    event_bus.unsubscribe<Core::Tasks::TaskCancelledEvent>(m_taskCancelledToken);

    LOG_DEBUG("ToastPlugin: Task events unsubscribed");
}

void ToastPlugin::on_task_started(const Core::Tasks::TaskStartedEvent& event) {
    // 检查任务名称是否以静默前缀开头
    // 这些任务不显示"开始"通知，避免骚扰用户
    static const std::vector<std::string> silent_prefixes = {
        "加载日志:",
        "Loading log:"
    };

    bool is_silent = false;
    for (const auto& prefix : silent_prefixes) {
        if (event.task_name.rfind(prefix, 0) == 0) {  // rfind with pos=0 检查前缀
            is_silent = true;
            break;
        }
    }

    if (!is_silent) {
        ToastManager::instance().info(
            "任务开始",
            std::format("正在执行: {}", event.task_name)
        );
    }
}

void ToastPlugin::on_task_completed(const Core::Tasks::TaskCompletedEvent& event) {
    // 检查任务名称是否以静默前缀开头
    // 这些任务不显示"完成"通知，避免骚扰用户
    static const std::vector<std::string> silent_prefixes = {
        "加载日志:",
        "Loading log:"
    };

    bool is_silent = false;
    for (const auto& prefix : silent_prefixes) {
        if (event.task_name.rfind(prefix, 0) == 0) {  // rfind with pos=0 检查前缀
            is_silent = true;
            break;
        }
    }

    if (!is_silent) {
        ToastManager::instance().success(
            "任务完成",
            std::format("{} 已成功完成 (耗时: {:.1f}ms)",
                      event.task_name,
                      event.duration_ms)
        );
    }
}

void ToastPlugin::on_task_failed(const Core::Tasks::TaskFailedEvent& event) {
    // 任务失败时显示错误气泡
    ToastManager::instance().error(
        "任务失败",
        std::format("{} 失败: {} (耗时: {:.1f}ms)", 
                  event.task_name, 
                  event.error_message,
                  event.duration_ms)
    );
}

void ToastPlugin::on_task_cancelled(const Core::Tasks::TaskCancelledEvent& event) {
    // 任务取消时显示警告气泡
    ToastManager::instance().warning(
        "任务已取消",
        std::format("{} 已被取消 (耗时: {:.1f}ms)", 
                  event.task_name, 
                  event.duration_ms)
    );
}

} // namespace DearTs::Plugins::Toast
