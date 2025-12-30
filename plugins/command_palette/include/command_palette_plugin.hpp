/**
 * @file command_palette_plugin.hpp
 * @brief 命令面板插件
 * @details 提供类似 VS Code 的命令面板功能
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/plugin/plugin.h"

namespace DearTs::Plugins::CommandPalette {

/**
 * @brief 命令面板插件类
 *
 * 提供 VS Code 风格的命令面板功能：
 * - 快捷键打开（Ctrl+Shift+P）
 * - 模糊搜索命令
 * - 键盘导航
 * - 命令执行
 */
class CommandPalettePlugin : public Core::Plugin::IPlugin {
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
};

} // namespace DearTs::Plugins::CommandPalette
