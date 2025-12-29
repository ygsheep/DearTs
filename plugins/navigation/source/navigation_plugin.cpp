/**
 * @file navigation_plugin.cpp
 * @brief 导航插件实现
 */

#include "navigation_plugin.hpp"
#include "views/sidebar_view.hpp"
#include "core/ui/view.h"
#include "core/content/registry_base.h"
#include "liblogger/logger.h"

using namespace DearTs::Core;

namespace DearTs::Plugins::Navigation {

Core::Plugin::PluginInfo NavigationPlugin::get_info() const {
    return Core::Plugin::PluginInfo{
        .name = "Navigation",
        .author = "DearTs Team",
        .description = "视图导航和管理系统，提供侧边栏和工具栏",
        .version = "1.0.0",
        .api_version = "1.0.0"
    };
}

Core::Result<void, std::string> NavigationPlugin::on_load() {
    LOG_INFO("Navigation plugin loading...");

    // 注册侧边栏视图
    ContentRegistry::Views::add<SidebarView>();
    LOG_INFO("  - SidebarView registered");

    LOG_INFO("Navigation plugin loaded successfully");
    return Core::Result<void, std::string>::ok();
}

void NavigationPlugin::on_unload() {
    LOG_INFO("Navigation plugin unloaded");
}

} // namespace DearTs::Plugins::Navigation

// 插件导出函数（仅在作为动态库时使用）
// 当作为内置插件编译时，这些函数会导致链接错误，因此被注释掉
/*
extern "C" {
    DearTs::Core::Plugin::IPlugin* dearts_create_plugin() {
        return new DearTs::Plugins::Navigation::NavigationPlugin();
    }

    void dearts_destroy_plugin(DearTs::Core::Plugin::IPlugin* plugin) {
        delete plugin;
    }

    const char* dearts_get_api_version() {
        return "1.0.0";
    }
}
*/
