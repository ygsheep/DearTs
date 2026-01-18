/**
 * @file clipboard_parser_plugin.cpp
 * @brief 剪切板解析插件实现
 */

#include "clipboard_parser_plugin.hpp"
#include "views/clipboard_parser_view.hpp"
#include "core/content/registry_base.h"
#include "core/content/commands.h"
#include "core/ui/shortcut_manager.h"
#include "core/tasks/task_manager.h"
#include "liblogger/logger.h"
#include "cppjieba/Jieba.hpp"
#include <imgui.h>
#include <SDL3/SDL.h>

using namespace DearTs::Core;

namespace DearTs::Plugins::ClipboardParser {

// 静态成员初始化
std::unique_ptr<cppjieba::Jieba> ClipboardParserPlugin::s_jieba = nullptr;
std::mutex ClipboardParserPlugin::s_jieba_mutex;
std::atomic<bool> ClipboardParserPlugin::s_jieba_loading{false};
std::atomic<bool> ClipboardParserPlugin::s_jieba_ready{false};

ClipboardParserPlugin::ClipboardParserPlugin() {
    LOG_INFO("ClipboardParserPlugin: Constructor called");
}

ClipboardParserPlugin::~ClipboardParserPlugin() {
    LOG_INFO("ClipboardParserPlugin: Destructor called");
}

Plugin::PluginInfo ClipboardParserPlugin::get_info() const {
    return Plugin::PluginInfo{
        .name = "ClipboardParser",
        .author = "DearTs Team",
        .description = "剪切板智能解析插件 - 监听剪切板变化，提供智能分词和内容提取功能",
        .version = "1.0.0",
        .api_version = "1.0.0"
    };
}

Result<void, std::string> ClipboardParserPlugin::on_load() {
    LOG_INFO("ClipboardParserPlugin: Loading...");

    // 注册剪切板解析视图
    ContentRegistry::Views::add<ClipboardParserView>();
    LOG_INFO("  - ClipboardParserView registered");

    // 注册命令
    ContentRegistry::Commands::Registry::instance().add(
        ContentRegistry::UnlocalizedString("clipboard_parser.toggle"),
        "打开剪切板解析器",
        []() {
            auto* view = ContentRegistry::Views::get_by_name(
                ContentRegistry::UnlocalizedString("剪切板解析器")
            );
            if (view) {
                view->get_window_open_state() = true;
                LOG_INFO("剪切板解析器 opened via command");
            }
        }
    );
    LOG_INFO("  - Command registered: clipboard_parser.toggle");

    // 启动后台加载 jieba
    start_jieba_background_load();

    LOG_INFO("ClipboardParserPlugin: Loaded successfully");
    return Result<void, std::string>::ok();
}

void ClipboardParserPlugin::on_unload() {
    LOG_INFO("ClipboardParserPlugin: Unloading...");
    // 清理资源（框架会自动取消订阅等）
    LOG_INFO("ClipboardParserPlugin: Unloaded");
}

void ClipboardParserPlugin::on_enable() {
    LOG_INFO("ClipboardParserPlugin: Enabled - Registering shortcuts");

    // 注册全局快捷键（默认 Alt+V）
    auto& manager = UI::ShortcutManager::instance();
    manager.addShortcut(
        "clipboard_parser.open",
        UI::Shortcut(ImGuiKey_V, false, false, true),  // Alt+V
        []() {
            auto* view = ContentRegistry::Views::get_by_name(
                ContentRegistry::UnlocalizedString("剪切板解析器")
            );
            if (view) {
                bool& state = view->get_window_open_state();
                state = !state;
                LOG_DEBUG("剪切板解析器 toggled via shortcut, new state: {}", state);
            }
        },
        UI::ShortcutType::Global
    );

    LOG_INFO("  - Registered shortcut: Alt+V (clipboard_parser.open)");
    LOG_INFO("ClipboardParserPlugin: Fully enabled");
}

void ClipboardParserPlugin::on_disable() {
    LOG_INFO("ClipboardParserPlugin: Disabled");
}

cppjieba::Jieba* ClipboardParserPlugin::get_jieba() {
    std::lock_guard<std::mutex> lock(s_jieba_mutex);
    return s_jieba.get();
}

bool ClipboardParserPlugin::is_jieba_ready() {
    return s_jieba_ready.load();
}

std::string ClipboardParserPlugin::get_jieba_dict_path() {
    const char* base_path = SDL_GetBasePath();
    std::string dict_dir;

    if (base_path) {
        std::string base_str(base_path);
        size_t pos = base_str.find("build");
        if (pos != std::string::npos) {
            dict_dir = base_str.substr(0, pos) + "third_party/cppjieba/dict";
        } else {
            dict_dir = base_str + "/../../../third_party/cppjieba/dict";
        }
    } else {
        dict_dir = "../../../third_party/cppjieba/dict";
    }

    return dict_dir;
}

void ClipboardParserPlugin::start_jieba_background_load() {
    // 如果已经在加载或已加载，不重复加载
    if (s_jieba_loading.load() || s_jieba_ready.load()) {
        LOG_INFO("ClipboardParserPlugin: Jieba already loading or ready, skipping");
        return;
    }

    s_jieba_loading.store(true);

    std::string dict_dir = get_jieba_dict_path();
    LOG_INFO("ClipboardParserPlugin: Starting background jieba load from: {}", dict_dir);

    // 使用 TaskManager 异步加载
    using namespace Tasks;

    TaskManager::instance().launch(
        "加载 Jieba 分词器",
        [dict_dir](const std::atomic<bool>& should_cancel) {
            try {
                LOG_INFO("ClipboardParserPlugin: Initializing Jieba in background...");

                auto jieba = std::make_unique<cppjieba::Jieba>(
                    dict_dir + "/jieba.dict.utf8",
                    dict_dir + "/hmm_model.utf8",
                    dict_dir + "/user.dict.utf8",
                    dict_dir + "/idf.utf8",
                    dict_dir + "/stop_words.utf8"
                );

                // 初始化完成，更新静态成员
                {
                    std::lock_guard<std::mutex> lock(s_jieba_mutex);
                    s_jieba = std::move(jieba);
                }
                s_jieba_ready.store(true);
                s_jieba_loading.store(false);

                LOG_INFO("ClipboardParserPlugin: Jieba initialized successfully in background");
            } catch (const std::exception& e) {
                LOG_ERROR("ClipboardParserPlugin: Failed to initialize Jieba: {}", e.what());
                s_jieba_loading.store(false);
                throw;
            }
        },
        TaskType::Background
    );
}

} // namespace DearTs::Plugins::ClipboardParser
