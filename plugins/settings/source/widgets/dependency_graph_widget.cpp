/**
 * @file dependency_graph_widget.cpp
 * @brief 插件依赖关系节点图可视化组件实现（简化版）
 */

#include "widgets/dependency_graph_widget.hpp"
#include "core/plugin/plugin.h"
#include "core/event/event_bus.h"
#include "liblogger/logger.h"
#include <imgui.h>
#include <format>
#include <algorithm>
#include <cmath>
#include <queue>

using namespace DearTs;
using namespace DearTs::Core;
using namespace DearTs::Plugins::Settings;

DependencyGraphWidget::DependencyGraphWidget()
    : m_initialized(false)
    , m_scroll{0.0f, 0.0f}
    , m_zoom(1.0f)
    , m_draggingNode(nullptr)
    , m_panning(false)
{
    subscribe_events();
    refresh_graph();
}

DependencyGraphWidget::~DependencyGraphWidget() {
    // 清理资源（如果有）
}

void DependencyGraphWidget::subscribe_events() {
    using namespace DearTs::Core::Event;
    using namespace DearTs::Core::Plugin;

    m_refreshToken = std::make_unique<EventToken>(
        EventBus::instance().subscribe<PluginListRefreshEvent>(
            [this](const PluginListRefreshEvent& e) {
                LOG_DEBUG("DependencyGraphWidget: Auto-refresh triggered, total plugins: {}", e.total_count);
                refresh_graph();
            }
        )
    );

    LOG_INFO("DependencyGraphWidget: Subscribed to plugin refresh events");
}

void DependencyGraphWidget::initialize_editor() {
    if (m_initialized) {
        return;
    }

    m_initialized = true;
    LOG_INFO("DependencyGraphWidget: Node editor initialized");
}

void DependencyGraphWidget::refresh_graph() {
    if (!m_initialized) {
        initialize_editor();
    }

    // 重新计算布局
    calculate_layout();

    LOG_INFO("DependencyGraphWidget: Refreshed graph");
}

void DependencyGraphWidget::build_graph() {
    // 在简化版实现中，不需要预先构建图
    // 直接在 render() 中绘制
}

void DependencyGraphWidget::calculate_layout() {
    // 使用 BFS 计算每个插件的层级
    std::unordered_map<std::string, int> levels;
    std::unordered_map<std::string, std::vector<std::string>> graph;
    std::unordered_map<std::string, int> inDegree;
    std::queue<std::string> queue;

    auto& pm = Plugin::PluginManager::instance();
    auto plugin_infos = pm.get_all_plugins_info();

    // 构建图和计算入度
    for (const auto& info : plugin_infos) {
        auto* plugin = pm.get_plugin(info.name);
        if (!plugin) continue;

        graph[info.name] = std::vector<std::string>();
        inDegree[info.name] = 0;
    }

    for (const auto& info : plugin_infos) {
        auto* plugin = pm.get_plugin(info.name);
        if (!plugin) continue;

        auto dependencies = plugin->get_dependencies();
        for (const auto& dep : dependencies) {
            if (pm.get_plugin(dep.plugin_name)) {
                graph[dep.plugin_name].push_back(info.name);
                inDegree[info.name]++;
            }
        }
    }

    // 从入度为 0 的节点开始 BFS
    for (const auto& [name, degree] : inDegree) {
        if (degree == 0) {
            queue.push(name);
            levels[name] = 0;
        }
    }

    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();

        for (const auto& neighbor : graph[current]) {
            if (levels.find(neighbor) == levels.end()) {
                levels[neighbor] = levels[current] + 1;
            } else {
                levels[neighbor] = std::max(levels[neighbor], levels[current] + 1);
            }
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                queue.push(neighbor);
            }
        }
    }

    // 存储节点位置
    m_nodePositions.clear();

    const float nodeWidth = 160.0f;
    const float nodeHeight = 60.0f;
    const float horizontalSpacing = 220.0f;
    const float verticalSpacing = 100.0f;

    // 按层级分组
    std::unordered_map<int, std::vector<std::string>> levelGroups;
    for (const auto& [name, level] : levels) {
        levelGroups[level].push_back(name);
    }

    // 为每个层级的节点设置位置
    for (const auto& [level, plugins] : levelGroups) {
        float x = level * horizontalSpacing + 50.0f;
        float y = 50.0f;

        for (const auto& pluginName : plugins) {
            m_nodePositions[pluginName] = NodePosition{x, y, nodeWidth, nodeHeight};
            y += verticalSpacing;
        }
    }
}

void DependencyGraphWidget::render() {
    if (!m_initialized) {
        initialize_editor();
    }

    auto& pm = Plugin::PluginManager::instance();
    auto plugin_infos = pm.get_all_plugins_info();

    if (plugin_infos.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "没有可显示的插件");
        return;
    }

    // 获取可用空间
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    // 创建画布子窗口（无边框，更简洁）
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::BeginChild("DependencyGraphCanvas", canvasSize, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);

    // 获取绘图列表
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasScreenPos = canvasPos;

    // 应用缩放和滚动偏移
    ImVec2 offset = ImVec2(m_scroll[0], m_scroll[1]);

    // 处理鼠标输入
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 localMousePos = ImVec2(
        (mousePos.x - canvasScreenPos.x - offset.x) / m_zoom,
        (mousePos.y - canvasScreenPos.y - offset.y) / m_zoom
    );

    // 鼠标滚轮缩放
    if (ImGui::IsWindowHovered()) {
        if (io.MouseWheel != 0.0f) {
            float oldZoom = m_zoom;
            m_zoom *= 1.0f + io.MouseWheel * 0.1f;
            m_zoom = std::clamp(m_zoom, 0.3f, 3.0f);

            // 以鼠标位置为中心缩放
            ImVec2 mouseDelta = ImVec2(
                (mousePos.x - canvasScreenPos.x) / oldZoom,
                (mousePos.y - canvasScreenPos.y) / oldZoom
            );
            m_scroll[0] -= mouseDelta.x * (m_zoom - oldZoom);
            m_scroll[1] -= mouseDelta.y * (m_zoom - oldZoom);
        }
    }

    // 中键或右键拖拽平移画布
    if (ImGui::IsWindowHovered() && (ImGui::IsMouseDown(2) || ImGui::IsMouseDown(1))) {
        m_panning = true;
        m_scroll[0] += io.MouseDelta.x;
        m_scroll[1] += io.MouseDelta.y;
    } else {
        m_panning = false;
    }

    // 绘制背景（点状网格）
    drawList->PushClipRect(canvasScreenPos, ImVec2(canvasScreenPos.x + canvasSize.x, canvasScreenPos.y + canvasSize.y), true);

    const float gridSize = 30.0f * m_zoom;
    const ImVec2 gridStart(
        canvasScreenPos.x + fmodf(offset.x, gridSize),
        canvasScreenPos.y + fmodf(offset.y, gridSize)
    );

    ImU32 gridColor = IM_COL32(60, 60, 60, 40);
    for (float x = gridStart.x; x < canvasScreenPos.x + canvasSize.x; x += gridSize) {
        drawList->AddLine(ImVec2(x, canvasScreenPos.y), ImVec2(x, canvasScreenPos.y + canvasSize.y), gridColor);
    }
    for (float y = gridStart.y; y < canvasScreenPos.y + canvasSize.y; y += gridSize) {
        drawList->AddLine(ImVec2(canvasScreenPos.x, y), ImVec2(canvasScreenPos.x + canvasSize.x, y), gridColor);
    }

    // 绘制所有连线（在节点下方）
    for (const auto& info : plugin_infos) {
        auto* plugin = pm.get_plugin(info.name);
        if (!plugin) continue;

        auto dependencies = plugin->get_dependencies();
        for (const auto& dep : dependencies) {
            auto itFrom = m_nodePositions.find(dep.plugin_name);
            auto itTo = m_nodePositions.find(info.name);

            if (itFrom != m_nodePositions.end() && itTo != m_nodePositions.end()) {
                const auto& from = itFrom->second;
                const auto& to = itTo->second;

                // 计算连线起点和终点（应用缩放和偏移）
                ImVec2 fromPos(
                    canvasScreenPos.x + (from.x + from.width) * m_zoom + offset.x,
                    canvasScreenPos.y + (from.y + from.height / 2) * m_zoom + offset.y
                );
                ImVec2 toPos(
                    canvasScreenPos.x + to.x * m_zoom + offset.x,
                    canvasScreenPos.y + (to.y + to.height / 2) * m_zoom + offset.y
                );

                // 绘制贝塞尔曲线
                ImU32 lineColor = IM_COL32(120, 120, 120, 200);
                float lineThickness = 2.0f * m_zoom;

                drawList->AddBezierCubic(
                    fromPos,
                    ImVec2(fromPos.x + 50 * m_zoom, fromPos.y),
                    ImVec2(toPos.x - 50 * m_zoom, toPos.y),
                    toPos,
                    lineColor,
                    lineThickness
                );
            }
        }
    }

    // 绘制所有节点
    for (const auto& info : plugin_infos) {
        auto it = m_nodePositions.find(info.name);
        if (it == m_nodePositions.end()) continue;

        const auto& pos = it->second;

        // 计算节点屏幕位置
        ImVec2 nodeMin(
            canvasScreenPos.x + pos.x * m_zoom + offset.x,
            canvasScreenPos.y + pos.y * m_zoom + offset.y
        );
        ImVec2 nodeMax(
            canvasScreenPos.x + (pos.x + pos.width) * m_zoom + offset.x,
            canvasScreenPos.y + (pos.y + pos.height) * m_zoom + offset.y
        );

        // 检查鼠标是否悬停在节点上
        bool isHovered = (localMousePos.x >= pos.x && localMousePos.x <= pos.x + pos.width &&
                         localMousePos.y >= pos.y && localMousePos.y <= pos.y + pos.height);

        // 节点拖拽
        if (isHovered && ImGui::IsMouseClicked(0) && !m_panning) {
            m_draggingNode = &m_nodePositions[info.name];
            m_dragStart = ImVec2(localMousePos.x - pos.x, localMousePos.y - pos.y);
        }

        if (m_draggingNode == &m_nodePositions[info.name]) {
            if (ImGui::IsMouseReleased(0)) {
                m_draggingNode = nullptr;
            } else if (ImGui::IsMouseDown(0)) {
                m_draggingNode->x = localMousePos.x - m_dragStart.x;
                m_draggingNode->y = localMousePos.y - m_dragStart.y;
            }
        }

        // 获取节点颜色（根据插件状态）
        ImVec4 nodeColor = get_node_color(info.name);
        ImU32 nodeColorU32 = ImGui::ColorConvertFloat4ToU32(nodeColor);

        // 绘制节点背景（圆角矩形，无边框）
        ImU32 bgColor = isHovered ? IM_COL32(70, 70, 70, 240) : IM_COL32(60, 60, 60, 230);
        ImU32 borderColor = isHovered ? nodeColorU32 : IM_COL32(80, 80, 80, 180);
        float rounding = 8.0f * m_zoom;

        drawList->AddRectFilled(nodeMin, nodeMax, bgColor, rounding);
        drawList->AddRect(nodeMin, nodeMax, borderColor, rounding, 0, isHovered ? 2.0f : 1.0f);

        // 绘制插件名称
        ImVec2 textSize = ImGui::CalcTextSize(info.name.c_str());
        ImVec2 textPos(
            nodeMin.x + (pos.width * m_zoom - textSize.x) / 2,
            nodeMin.y + 10 * m_zoom
        );
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), info.name.c_str());

        // 绘制插件状态（使用文本标识，避免 emoji 乱码）
        auto state_result = pm.get_plugin_state(info.name);
        const char* stateText = "?";
        if (state_result.isOk()) {
            auto state = state_result.unwrap();
            switch (state) {
                case Plugin::PluginState::Enabled: stateText = "运行中"; break;
                case Plugin::PluginState::Loaded: stateText = "已加载"; break;
                case Plugin::PluginState::Disabled: stateText = "已禁用"; break;
                case Plugin::PluginState::Unloaded: stateText = "未加载"; break;
                case Plugin::PluginState::Error: stateText = "错误"; break;
            }
        }

        textSize = ImGui::CalcTextSize(stateText);
        textPos = ImVec2(
            nodeMin.x + (pos.width * m_zoom - textSize.x) / 2,
            nodeMin.y + 35 * m_zoom
        );
        drawList->AddText(textPos, ImGui::ColorConvertFloat4ToU32(nodeColor), stateText);
    }

    drawList->PopClipRect();

    ImGui::EndChild();

    // 绘制控制提示
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "提示: 滚轮缩放 | 右键拖动画布 | 左键拖拽节点");
}

void DependencyGraphWidget::set_selected_plugin(const std::string& plugin_name) {
    m_selectedPlugin = plugin_name;
    // TODO: 高亮选中的插件节点
}

ImVec4 DependencyGraphWidget::get_node_color(const std::string& plugin_name) {
    auto& pm = Plugin::PluginManager::instance();
    auto state_result = pm.get_plugin_state(plugin_name);

    if (state_result.isOk()) {
        auto state = state_result.unwrap();
        switch (state) {
            case Plugin::PluginState::Enabled: return ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
            case Plugin::PluginState::Loaded: return ImVec4(0.2f, 0.6f, 0.8f, 1.0f);
            case Plugin::PluginState::Disabled: return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
            case Plugin::PluginState::Unloaded: return ImVec4(0.9f, 0.5f, 0.0f, 1.0f);
            case Plugin::PluginState::Error: return ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
        }
    }

    return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
}

int DependencyGraphWidget::get_plugin_level(const std::string& plugin_name) {
    // 在简化版实现中，不需要单独的层级查询
    return 0;
}
