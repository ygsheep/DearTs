/**
 * @file settings_plugin.hpp
 * @brief 设置插件声明
 * @details 提供 ConfigManager 的图形界面管理
 */

#pragma once

#include "core/plugin/plugin.h"

namespace DearTs::Plugins::Settings {

/**
 * @brief 设置插件类
 *
 * 提供 DearTs 的设置管理功能：
 * - 配置视图
 * - 配置编辑器
 * - 系统功能设置
 */
class SettingsPlugin : public Core::Plugin::IPlugin {
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
     * @brief 注册所有视图
     */
    void register_views();
};

} // namespace DearTs::Plugins::Settings
