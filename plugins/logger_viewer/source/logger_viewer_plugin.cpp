/**
 * @file logger_viewer_plugin.cpp
 * @brief 日志查看器插件实现
 */

#include "logger_viewer_plugin.hpp"
#include "views/logger_viewer_view.hpp"
#include "core/ui/view.h"
#include "core/content/registry_base.h"
#include "liblogger/logger.h"

using namespace DearTs::Core;

namespace DearTs::Plugins::LoggerViewer {

Core::Plugin::PluginInfo LoggerViewerPlugin::get_info() const {
    return Core::Plugin::PluginInfo{
        .name = "LoggerViewer",
        .author = "DearTs Team",
        .description = "日志文件查看、筛选、搜索和分析工具",
        .version = "1.0.0",
        .api_version = "1.0.0"
    };
}

Result<void, std::string> LoggerViewerPlugin::on_load() {
    LOG_INFO("LoggerViewer plugin loading...");

    // 注册日志查看器视图
    ContentRegistry::Views::add<LoggerViewerView>();
    LOG_INFO("  - LoggerViewer View registered");

    LOG_INFO("LoggerViewer plugin loaded successfully");
    return Result<void, std::string>::ok();
}

void LoggerViewerPlugin::on_unload() {
    LOG_INFO("LoggerViewer plugin unloaded");
}

} // namespace DearTs::Plugins::LoggerViewer

// 插件导出函数（仅在作为动态库时使用）
// 当作为内置插件编译时，这些函数会导致链接错误，因此被注释掉
/*
extern "C" {
    DearTs::Core::Plugin::IPlugin* dearts_create_plugin() {
        return new DearTs::Plugins::LoggerViewer::LoggerViewerPlugin();
    }

    void dearts_destroy_plugin(DearTs::Core::Plugin::IPlugin* plugin) {
        delete plugin;
    }

    const char* dearts_get_api_version() {
        return "1.0.0";
    }
}
*/
