/**
 * @file wuthering_waves_plugin.hpp
 * @brief 鸣潮抽卡记录插件
 */

#pragma once

#include "core/plugin/plugin.h"

namespace DearTs::Plugins::WutheringWaves {

/**
 * @brief 鸣潮抽卡记录插件
 *
 * @details 功能：
 * - 自动查找《鸣潮》游戏安装路径
 * - 从游戏日志中提取抽卡记录 URL
 * - 一键复制 URL 到剪贴板
 * - 支持手动指定游戏路径
 */
class WutheringWavesPlugin : public Core::Plugin::IPlugin {
public:
    WutheringWavesPlugin() = default;
    ~WutheringWavesPlugin() override = default;

    /**
     * @brief 获取插件信息
     */
    Core::Plugin::PluginInfo get_info() const override {
        return Core::Plugin::PluginInfo{
            .name = "WutheringWaves",
            .author = "DearTs Team",
            .description = "鸣潮抽卡记录获取工具 - 自动提取抽卡记录 URL",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

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

} // namespace DearTs::Plugins::WutheringWaves
