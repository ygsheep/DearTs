/**
 * @file settings_plugin.cpp
 * @brief 设置插件实现
 * @details 这个插件提供 DearTs 的设置管理界面
 */

#include "settings_plugin.hpp"
#include "views/settings_view.hpp"
#include "core/ui/view.h"
#include "core/content/registry_base.h"
#include "liblogger/logger.h"

using namespace DearTs::Core;

namespace DearTs::Plugins::Settings {

Plugin::PluginInfo SettingsPlugin::get_info() const {
    return Plugin::PluginInfo{
        .name = "Settings",
        .author = "DearTs Team",
        .description = "DearTs 设置管理插件，提供配置编辑和系统设置",
        .version = "1.0.0",
        .api_version = "1.0.0"
    };
}

Result<void, std::string> SettingsPlugin::on_load() {
    LOG_INFO("SettingsPlugin: Loading...");

    // 注册视图
    register_views();

    LOG_INFO("SettingsPlugin: Loaded successfully");
    return Result<void, std::string>::ok();
}

void SettingsPlugin::on_unload() {
    LOG_INFO("SettingsPlugin: Unloading...");
    LOG_INFO("SettingsPlugin: Unloaded");
}

void SettingsPlugin::on_enable() {
    LOG_INFO("SettingsPlugin: Enabled");
}

void SettingsPlugin::on_disable() {
    LOG_INFO("SettingsPlugin: Disabled");
}

void SettingsPlugin::register_views() {
    LOG_INFO("SettingsPlugin: Registering views...");

    // 注册设置视图
    ContentRegistry::Views::add<SettingsView>();
    LOG_INFO("  - Settings View registered");

    // 默认不打开设置窗口（用户可以通过菜单或快捷键打开）
    // 如果想默认打开，取消下面的注释：
    /*
    auto* settings_view = ContentRegistry::Views::get_by_name(ContentRegistry::UnlocalizedString("Settings"));
    if (settings_view != nullptr) {
        settings_view->get_window_open_state() = true;
        LOG_INFO("    - Settings opened by default");
    }
    */

    LOG_INFO("SettingsPlugin: All views registered");
}

} // namespace DearTs::Plugins::Settings

// 注意：此插件作为静态内置插件编译，不需要导出函数
// 导出函数仅用于动态加载的插件（.dll/.so）
// 对于内置插件，直接在 toolbox_application.cpp 中通过 add_builtin() 加载
