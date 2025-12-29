/**
 * @file navigation_plugin.hpp
 * @brief 导航插件
 * @details 提供侧边栏和工具栏，用于管理视图的显示和隐藏
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/plugin/plugin.h"
#include "core/result.h"

namespace DearTs::Plugins::Navigation {

/**
 * @brief 导航插件类
 */
class NavigationPlugin : public Core::Plugin::IPlugin {
public:
    NavigationPlugin() = default;
    ~NavigationPlugin() override = default;

    /**
     * @brief 获取插件信息
     */
    [[nodiscard]] Core::Plugin::PluginInfo get_info() const override;

    /**
     * @brief 插件加载时调用
     */
    Core::Result<void, std::string> on_load() override;

    /**
     * @brief 插件卸载时调用
     */
    void on_unload() override;
};

} // namespace DearTs::Plugins::Navigation
