/**
 * @file logger_viewer_plugin.hpp
 * @brief 日志查看器插件
 * @details 提供日志文件查看、筛选、搜索等功能的插件
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/plugin/plugin.h"
#include "core/result.h"

namespace DearTs::Plugins::LoggerViewer {

/**
 * @brief 日志查看器插件类
 */
class LoggerViewerPlugin : public Core::Plugin::IPlugin {
public:
    LoggerViewerPlugin() = default;
    ~LoggerViewerPlugin() override = default;

    /**
     * @brief 获取插件信息
     */
    [[nodiscard]] Core::Plugin::PluginInfo get_info() const override;

    /**
     * @brief 插件加载时调用
     * @details 在此注册视图、命令等
     */
    Core::Result<void, std::string> on_load() override;

    /**
     * @brief 插件卸载时调用
     */
    void on_unload() override;
};

} // namespace DearTs::Plugins::LoggerViewer
