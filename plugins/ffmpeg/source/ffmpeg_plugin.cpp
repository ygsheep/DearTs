/**
 * @file ffmpeg_plugin.cpp
 * @brief FFmpeg 插件实现
 */

#include "ffmpeg_plugin.hpp"
#include "ffmpeg_view.hpp"
#include "liblogger/logger.h"
#include "core/content/registry_base.h"
#include "core/ui/view.h"

using namespace DearTs::Core;

namespace DearTs::Plugins::FFmpeg {

Plugin::PluginInfo FFmpegPlugin::get_info() const {
    return Plugin::PluginInfo{
        .name = "FFmpeg",
        .author = "DearTs Team",
        .description = "基于FFmpeg的视频处理工具",
        .version = "1.0.0",
        .api_version = "1.0.0"
    };
}

Result<void, std::string> FFmpegPlugin::on_load() {
    LOG_INFO("FFmpeg Plugin: Loading...");

    // 注册视图
    ContentRegistry::Views::add<FFmpegView>();

    LOG_INFO("FFmpeg Plugin: Loaded successfully");
    return Result<void, std::string>::ok();
}

void FFmpegPlugin::on_unload() {
    LOG_INFO("FFmpeg Plugin: Unloading...");

    // 移除注册的视图（注意：视图名称是 "合并TS文件" 而不是 "FFmpeg"）
    ContentRegistry::Views::remove("合并TS文件");

    LOG_INFO("FFmpeg Plugin: Unloaded");
}

void FFmpegPlugin::on_enable() {
    LOG_INFO("FFmpeg Plugin: Enabled");
}

void FFmpegPlugin::on_disable() {
    LOG_INFO("FFmpeg Plugin: Disabled");
}

} // namespace DearTs::Plugins::FFmpeg
