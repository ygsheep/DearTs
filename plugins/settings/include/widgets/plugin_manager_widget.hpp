/**
 * @file plugin_manager_widget.hpp
 * @brief 插件管理器组件
 * @details 显示所有插件的状态、信息和依赖关系
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "core/event/event_bus.h"
#include "core/plugin/plugin.h"
#include "widgets/dependency_graph_widget.hpp"

// 前向声明
struct ImVec4;

namespace DearTs::Plugins::Settings {

/**
 * @brief 插件元数据事件
 */
struct PluginMetadataEvent {
    std::string name;
    std::string author;
    std::string description;
    std::string version;
    std::string state;  // Unloaded, Loaded, Enabled, Disabled, Error
    bool is_builtin;    // 是否为内置插件
};

/**
 * @brief 插件管理器组件
 *
 * 负责渲染和管理插件相关的界面：
 * - 插件列表（左侧面板）
 * - 插件详情（右侧面板）
 * - 依赖关系图（右侧标签页）
 */
class PluginManagerWidget {
public:
    PluginManagerWidget();
    ~PluginManagerWidget() = default;

    /**
     * @brief 渲染插件管理器面板
     */
    void render();

    /**
     * @brief 刷新插件列表
     */
    void refresh_plugins();

private:
    /**
     * @brief 绘制插件列表（左侧面板）
     */
    void draw_plugin_list();

    /**
     * @brief 绘制插件详情（右侧标签页）
     */
    void draw_plugin_details();

    /**
     * @brief 绘制依赖关系图（右侧标签页）
     */
    void draw_dependency_graph();

    /**
     * @brief 绘制状态徽章
     */
    void draw_state_badge(const std::string& state);

    /**
     * @brief 获取状态的显示文本
     */
    const char* get_state_text(const std::string& state);

    /**
     * @brief 获取状态的颜色
     */
    ImVec4 get_state_color(const std::string& state);

    /**
     * @brief 订阅插件事件
     */
    void subscribe_events();

    // 插件元数据
    struct PluginInfo {
        std::string name;
        std::string author;
        std::string description;
        std::string version;
        std::string state;
        bool is_builtin;
    };

    std::vector<PluginInfo> m_plugins;
    int m_selected_plugin = -1;

    // 事件订阅令牌
    std::unique_ptr<Core::Event::EventToken> m_refreshToken;

    // 依赖关系图组件
    std::unique_ptr<DependencyGraphWidget> m_dependencyGraph;
};

} // namespace DearTs::Plugins::Settings
