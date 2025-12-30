/**
 * @file command_palette_plugin.cpp
 * @brief 命令面板插件实现
 */

#include "command_palette_plugin.hpp"
#include "command_palette_view.hpp"
#include "core/content/registry_base.h"
#include "core/ui/shortcut_manager.h"
#include "liblogger/logger.h"
#include <imgui.h>

using namespace DearTs::Core;

namespace DearTs::Plugins::CommandPalette {

Plugin::PluginInfo CommandPalettePlugin::get_info() const {
    return Plugin::PluginInfo{
        .name = "CommandPalette",
        .author = "DearTs Team",
        .description = "命令面板插件，提供类似 VS Code 的命令执行界面",
        .version = "1.0.0",
        .api_version = "1.0.0"
    };
}

Result<void, std::string> CommandPalettePlugin::on_load() {
    LOG_INFO("CommandPalettePlugin: Loading...");

    // 注册命令面板视图
    ContentRegistry::Views::add<CommandPaletteView>();
    LOG_INFO("  - CommandPaletteView registered");

    // 注册命令面板专用命令
    ContentRegistry::Commands::add(
        "command_palette.toggle",
        "打开命令面板",
        []() {
            auto* view = ContentRegistry::Views::get_by_name(
                ContentRegistry::UnlocalizedString("命令面板")
            );
            if (view) {
                view->get_window_open_state() = true;
                LOG_INFO("命令面板 opened via command");
            }
        }
    );

    LOG_INFO("CommandPalettePlugin: Loaded successfully");
    return Result<void, std::string>::ok();
}

void CommandPalettePlugin::on_unload() {
    LOG_INFO("CommandPalettePlugin: Unloading...");
    // 清理资源（框架会自动取消订阅等）
    LOG_INFO("CommandPalettePlugin: Unloaded");
}

void CommandPalettePlugin::on_enable() {
    LOG_INFO("CommandPalettePlugin: Enabled - Registering shortcuts");

    // 注册全局快捷键 Ctrl+Shift+P
    auto& manager = UI::ShortcutManager::instance();
    manager.addShortcut(
        "command_palette.open",
        UI::Shortcut(ImGuiKey_P, true, true, false),  // Ctrl+Shift+P
        []() {
            auto* view = ContentRegistry::Views::get_by_name(
                ContentRegistry::UnlocalizedString("命令面板")
            );
            if (view) {
                bool& state = view->get_window_open_state();
                state = !state;
                LOG_DEBUG("命令面板 toggled via shortcut, new state: {}", state);
            }
        },
        UI::ShortcutType::Global
    );

    LOG_INFO("  - Registered shortcut: Ctrl+Shift+P (command_palette.open)");
}

void CommandPalettePlugin::on_disable() {
    LOG_INFO("CommandPalettePlugin: Disabled");
}

} // namespace DearTs::Plugins::CommandPalette
