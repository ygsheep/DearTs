/**
 * @file dependency_graph_widget.hpp
 * @brief 插件依赖关系节点图可视化组件（简化版）
 * @details 使用 ImGui 绘制插件依赖关系图
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "core/plugin/plugin.h"
#include "core/event/event_bus.h"
#include <imgui.h>

// 前向声明
struct ImVec4;

namespace DearTs::Plugins::Settings {

/**
 * @brief 节点位置信息
 */
struct NodePosition {
    float x;
    float y;
    float width;
    float height;
};

/**
 * @brief 插件依赖关系节点图可视化组件
 *
 * 使用 ImGui 的绘图功能绘制插件依赖关系图：
 * - 插件作为节点显示
 * - 依赖关系作为连线显示
 * - 支持缩放、拖拽
 * - 自动层次化布局
 */
class DependencyGraphWidget {
public:
    DependencyGraphWidget();
    ~DependencyGraphWidget();

    /**
     * @brief 渲染节点图
     */
    void render();

    /**
     * @brief 刷新节点图数据
     */
    void refresh_graph();

    /**
     * @brief 设置当前选中的插件
     * @param plugin_name 插件名称
     */
    void set_selected_plugin(const std::string& plugin_name);

private:
    /**
     * @brief 订阅插件事件
     */
    void subscribe_events();

    /**
     * @brief 初始化节点编辑器
     */
    void initialize_editor();

    /**
     * @brief 构建节点图
     */
    void build_graph();

    /**
     * @brief 计算层次化布局
     */
    void calculate_layout();

    /**
     * @brief 获取节点颜色（根据插件状态）
     */
    ImVec4 get_node_color(const std::string& plugin_name);

    /**
     * @brief 获取插件在依赖图中的层级（用于布局）
     */
    int get_plugin_level(const std::string& plugin_name);

    // 节点位置映射
    std::unordered_map<std::string, NodePosition> m_nodePositions;

    // 视图状态
    float m_scroll[2];           // 画布滚动偏移
    float m_zoom;                // 缩放级别
    NodePosition* m_draggingNode;
    ImVec2 m_dragStart;         // 拖拽起始位置
    bool m_panning;              // 是否正在平移画布

    // 当前选中的插件
    std::string m_selectedPlugin;

    // 编辑器是否已初始化
    bool m_initialized;

    // 事件订阅令牌
    std::unique_ptr<Core::Event::EventToken> m_refreshToken;
};

} // namespace DearTs::Plugins::Settings
