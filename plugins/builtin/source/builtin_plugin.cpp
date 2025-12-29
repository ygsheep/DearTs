/**
 * @file builtin_plugin.cpp
 * @brief 内置插件实现
 * @details 这个插件提供 DearTs 的默认 UI 组件
 */

#include "builtin_plugin.hpp"
#include "views/hello_world_view.hpp"
#include "views/data_inspector_view.hpp"
#include "core/ui/view.h"
#include "core/content/registry_base.h"
#include "liblogger/logger.h"

using namespace DearTs::Core;

namespace DearTs::Plugins::Builtin {

Core::Plugin::PluginInfo BuiltinPlugin::get_info() const {
    return Core::Plugin::PluginInfo{
        .name = "Builtin",
        .author = "DearTs Team",
        .description = "DearTs 内置插件，提供默认 UI 组件和功能",
        .version = "1.0.0",
        .api_version = "1.0.0"
    };
}

Core::Result<void, std::string> BuiltinPlugin::on_load() {
    LOG_INFO("BuiltinPlugin: Loading...");

    // 注册视图
    register_views();

    LOG_INFO("BuiltinPlugin: Loaded successfully");
    return Core::Result<void, std::string>::ok();
}

void BuiltinPlugin::on_unload() {
    LOG_INFO("BuiltinPlugin: Unloading...");
    // 清理资源（框架会自动取消订阅等）
    LOG_INFO("BuiltinPlugin: Unloaded");
}

void BuiltinPlugin::on_enable() {
    LOG_INFO("BuiltinPlugin: Enabled");
}

void BuiltinPlugin::on_disable() {
    LOG_INFO("BuiltinPlugin: Disabled");
}

void BuiltinPlugin::register_views() {
    LOG_INFO("BuiltinPlugin: Registering views...");

    // 注册 Data Inspector 视图
    ContentRegistry::Views::add<DataInspectorView>();
    LOG_INFO("  - Data Inspector View registered");

    // 默认打开 Data Inspector 视图
    auto* data_view = ContentRegistry::Views::get_by_name(ContentRegistry::UnlocalizedString("Data Inspector"));
    if (data_view != nullptr) {
        data_view->get_window_open_state() = true;
        LOG_INFO("    - Data Inspector opened by default");
    }

    // Hello World 视图不再注册，但保留源文件供参考

    LOG_INFO("BuiltinPlugin: All views registered");
}

} // namespace DearTs::Plugins::Builtin

// 注意：此插件作为静态内置插件编译，不需要导出函数
// 导出函数仅用于动态加载的插件（.dll/.so）
// 对于内置插件，直接在 toolbox_application.cpp 中通过 add_builtin() 加载
