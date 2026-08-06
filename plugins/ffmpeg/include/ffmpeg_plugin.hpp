/**
 * @file ffmpeg_plugin.hpp
 * @brief FFmpeg 视频处理插件
 * @details 提供多种基于 FFmpeg 的视频处理功能
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/plugin/plugin.h"

namespace DearTs::Plugins::FFmpeg {

/**
 * @brief FFmpeg 视频处理插件类
 *
 * 功能模块：
 * - TS 文件合并（第一个功能）
 * - 未来可扩展：视频转码、剪辑、格式转换等
 */
class FFmpegPlugin : public Core::Plugin::IPlugin {
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

} // namespace DearTs::Plugins::FFmpeg
