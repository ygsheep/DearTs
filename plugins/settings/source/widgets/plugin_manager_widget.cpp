/**
 * @file plugin_manager_widget.cpp
 * @brief 插件管理器组件实现
 */

#include "widgets/plugin_manager_widget.hpp"
#include "widgets/dependency_graph_widget.hpp"
#include "core/plugin/plugin.h"
#include "core/plugin/dependency_resolver.h"
#include "core/content/commands.h"
#include "core/event/event_bus.h"
#include "liblogger/logger.h"
#include <imgui.h>
#include <format>

using namespace DearTs;
using namespace DearTs::Core;
using namespace DearTs::Plugins::Settings;

PluginManagerWidget::PluginManagerWidget() {
    subscribe_events();
    refresh_plugins();

    // 创建依赖关系图组件
    m_dependencyGraph = std::make_unique<DependencyGraphWidget>();
}

void PluginManagerWidget::subscribe_events() {
    // 订阅插件列表刷新事件，实现自动刷新
    using namespace DearTs::Core::Event;
    using namespace DearTs::Core::Plugin;

    m_refreshToken = std::make_unique<EventToken>(
        EventBus::instance().subscribe<PluginListRefreshEvent>(
            [this](const PluginListRefreshEvent& e) {
                LOG_DEBUG("PluginManagerWidget: Auto-refresh triggered, total plugins: {}", e.total_count);
                refresh_plugins();
            }
        )
    );

    LOG_INFO("PluginManagerWidget: Subscribed to plugin refresh events");
}

void PluginManagerWidget::refresh_plugins() {
    m_plugins.clear();

    // 从 PluginManager 获取所有插件信息
    auto& pm = Plugin::PluginManager::instance();
    auto plugin_infos = pm.get_all_plugins_info();

    for (const auto& info : plugin_infos) {
        // 获取插件状态
        auto state_result = pm.get_plugin_state(info.name);
        std::string state = "Unknown";
        if (state_result.isOk()) {
            switch (state_result.unwrap()) {
                case Plugin::PluginState::Unloaded: state = "Unloaded"; break;
                case Plugin::PluginState::Loaded: state = "Loaded"; break;
                case Plugin::PluginState::Enabled: state = "Enabled"; break;
                case Plugin::PluginState::Disabled: state = "Disabled"; break;
                case Plugin::PluginState::Error: state = "Error"; break;
            }
        }

        m_plugins.push_back(PluginInfo{
            .name = info.name,
            .author = info.author,
            .description = info.description,
            .version = info.version,
            .state = state,
            .is_builtin = pm.is_plugin_builtin(info.name)
        });
    }

    LOG_INFO("PluginManagerWidget: Refreshed {} plugins", m_plugins.size());
}

void PluginManagerWidget::render() {
    // 顶部工具栏
    if (ImGui::Button("刷新")) {
        refresh_plugins();
    }
    ImGui::SameLine();
    ImGui::Text("共 %zu 个插件", m_plugins.size());

    // 依赖解析模式切换
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    auto& pm = Plugin::PluginManager::instance();
    auto current_mode = pm.get_dependency_mode();

    ImGui::Text("依赖解析模式:");
    ImGui::SameLine();

    const char* mode_items[] = {"宽松", "严格"};
    int current_mode_index = (current_mode == Plugin::DependencyResolutionMode::Strict) ? 1 : 0;

    ImGui::SetNextItemWidth(100);
    if (ImGui::Combo("##DependencyMode", &current_mode_index, mode_items, 2)) {
        auto new_mode = (current_mode_index == 1) ?
                        Plugin::DependencyResolutionMode::Strict :
                        Plugin::DependencyResolutionMode::Lenient;
        pm.set_dependency_mode(new_mode);
        LOG_INFO("Dependency resolution mode changed to: {}",
                 (new_mode == Plugin::DependencyResolutionMode::Strict) ? "Strict" : "Lenient");
    }

    // 添加工具提示
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "宽松模式: 加载所有可用的插件，忽略依赖错误\n"
            "严格模式: 遇到依赖错误时停止加载插件"
        );
    }

    ImGui::Separator();

    // 使用 Child 实现左右分栏布局
    static float left_width = 400.0f;

    // 计算可用空间
    ImVec2 content_avail = ImGui::GetContentRegionAvail();

    // 左侧：插件列表
    ImGui::BeginChild("LeftPanel", ImVec2(left_width, content_avail.y), true);
    draw_plugin_list();
    ImGui::EndChild();

    ImGui::SameLine();

    // 右侧：插件详情或依赖图
    ImGui::BeginChild("RightPanel", ImVec2(content_avail.x - left_width - 10, content_avail.y), true);
    if (m_selected_plugin >= 0 && m_selected_plugin < static_cast<int>(m_plugins.size())) {
        ImGui::BeginTabBar("PluginTabs");

        if (ImGui::BeginTabItem("详情")) {
            draw_plugin_details();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("依赖关系")) {
            draw_dependency_graph();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);
        ImGui::TextWrapped("选择一个插件查看详情");
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
}

void PluginManagerWidget::draw_plugin_list() {
    ImGui::Text("插件列表");

    if (ImGui::BeginListBox("##PluginList", ImVec2(-1, -1))) {
        for (int i = 0; i < static_cast<int>(m_plugins.size()); i++) {
            const auto& plugin = m_plugins[i];

            bool is_selected = (m_selected_plugin == i);

            // 插件名称行
            ImGui::PushID(i);

            if (ImGui::Selectable(("##" + plugin.name).c_str(), is_selected,
                                 ImGuiSelectableFlags_AllowDoubleClick)) {
                m_selected_plugin = i;
                if (ImGui::IsMouseDoubleClicked(0)) {
                    // 双击打开详情（TODO）
                }
            }

            // 绘制状态徽章
            ImGui::SameLine();
            draw_state_badge(plugin.state);

            // 插件名称和版本
            ImGui::SameLine();
            ImGui::Text("%s v%s", plugin.name.c_str(), plugin.version.c_str());

            // 描述
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", plugin.description.c_str());

            ImGui::PopID();
        }

        ImGui::EndListBox();
    }
}

void PluginManagerWidget::draw_plugin_details() {
    if (m_selected_plugin < 0 || m_selected_plugin >= static_cast<int>(m_plugins.size())) {
        return;
    }

    const auto& plugin = m_plugins[m_selected_plugin];

    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);

    // 插件名称和徽章
    ImGui::Text("插件名称");
    ImGui::SameLine();
    draw_state_badge(plugin.state);
    ImGui::Separator();

    ImGui::Text("名称: %s", plugin.name.c_str());
    ImGui::Text("版本: %s", plugin.version.c_str());
    ImGui::Text("作者: %s", plugin.author.c_str());
    ImGui::Text("状态: %s", get_state_text(plugin.state));
    ImGui::Text("类型: %s", plugin.is_builtin ? "内置插件" : "动态插件");

    ImGui::Separator();
    ImGui::Text("描述");
    ImGui::TextWrapped("%s", plugin.description.c_str());

    ImGui::PopTextWrapPos();

    // 操作按钮区域
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("操作");

    auto& pm = Plugin::PluginManager::instance();
    auto state_result = pm.get_plugin_state(plugin.name);

    // 根据插件状态显示不同的操作按钮
    if (state_result.isOk()) {
        auto state = state_result.unwrap();

        // 启用/禁用按钮
        if (state == Plugin::PluginState::Enabled) {
            if (ImGui::Button("禁用插件")) {
                auto result = pm.disable(plugin.name);
                if (result.isOk()) {
                    LOG_INFO("Disabled plugin: {}", plugin.name);
                    refresh_plugins();
                } else {
                    LOG_ERROR("Failed to disable plugin '{}': {}", plugin.name, result.error());
                                        }
            }
        } else if (state == Plugin::PluginState::Loaded || state == Plugin::PluginState::Disabled) {
            if (ImGui::Button("启用插件")) {
                auto result = pm.enable(plugin.name);
                if (result.isOk()) {
                    LOG_INFO("Enabled plugin: {}", plugin.name);
                    refresh_plugins();
                } else {
                    LOG_ERROR("Failed to enable plugin '{}': {}", plugin.name, result.error());
                }
            }
        }

        // 卸载按钮（仅动态插件）
        if (!plugin.is_builtin && (state == Plugin::PluginState::Loaded || state == Plugin::PluginState::Disabled || state == Plugin::PluginState::Error)) {
            ImGui::SameLine();
            if (ImGui::Button("卸载插件")) {
                if (pm.unload(plugin.name)) {
                    LOG_INFO("Unloaded plugin: {}", plugin.name);
                    refresh_plugins();
                } else {
                    LOG_WARN("Failed to unload plugin: {}", plugin.name);
                }
            }
        }

        // 重载按钮
        ImGui::SameLine();
        if (ImGui::Button("重载插件")) {
            auto result = pm.reload(plugin.name);
            if (result.isOk()) {
                LOG_INFO("Reloaded plugin: {}", plugin.name);
                refresh_plugins();
            } else {
                LOG_WARN("Reload not implemented or failed: {}", result.error());
            }
        }
    }

    // 依赖关系信息
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("依赖关系");

    // 获取插件的依赖信息
    auto* plugin_ptr = pm.get_plugin(plugin.name);
    if (plugin_ptr) {
        auto dependencies = plugin_ptr->get_dependencies();

        if (dependencies.empty()) {
            ImGui::TextDisabled("此插件没有依赖");
        } else {
            // 依赖列表表格
            if (ImGui::BeginTable("DependenciesTable", 4,
                                 ImGuiTableFlags_Borders |
                                 ImGuiTableFlags_RowBg |
                                 ImGuiTableFlags_SizingFixedFit |
                                 ImGuiTableFlags_Resizable |
                                 ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupScrollFreeze(0, 1); // 固定表头
                ImGui::TableSetupColumn("依赖名称", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("类型", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("版本要求", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("状态", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableHeadersRow();

                for (const auto& dep : dependencies) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", dep.plugin_name.c_str());

                    ImGui::TableSetColumnIndex(1);
                    const char* type_str = "";
                    ImVec4 type_color = ImVec4(1, 1, 1, 1);
                    switch (dep.type) {
                        case Plugin::DependencyType::Required:
                            type_str = "必需";
                            type_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // 红色
                            break;
                        case Plugin::DependencyType::Optional:
                            type_str = "可选";
                            type_color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f); // 橙色
                            break;
                        case Plugin::DependencyType::Soft:
                            type_str = "软依赖";
                            type_color = ImVec4(0.5f, 0.8f, 1.0f, 1.0f); // 蓝色
                            break;
                    }
                    ImGui::TextColored(type_color, "%s", type_str);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", dep.version_range.to_string().c_str());

                    ImGui::TableSetColumnIndex(3);
                    // 检查依赖状态
                    auto dep_state_result = pm.get_plugin_state(dep.plugin_name);
                    if (dep_state_result.isOk()) {
                        auto dep_state = dep_state_result.unwrap();
                        ImVec4 state_color = get_state_color_by_state(dep_state);
                        const char* state_text = get_state_text_by_state(dep_state);
                        ImGui::TextColored(state_color, "%s", state_text);
                    } else {
                        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "未找到");
                    }
                }

                ImGui::EndTable();
            }
        }
    } else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "无法获取插件依赖信息");
    }

    // 依赖解析功能
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("依赖解析");

    // 解析按钮
    static bool show_resolution_result = false;
    if (ImGui::Button("解析依赖关系")) {
        // 触发依赖解析
        auto plugins = pm.get_all_plugins_info();
        std::unordered_map<std::string, Plugin::IPlugin*> plugin_map;
        for (const auto& info : plugins) {
            auto* plugin = pm.get_plugin(info.name);
            if (plugin) {
                plugin_map[info.name] = plugin;
            }
        }

        auto mode = pm.get_dependency_mode();
        auto result = Plugin::DependencyResolver::resolve(plugin_map, mode);

        show_resolution_result = true;

        // 显示结果
        if (result.success) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "✓ 依赖解析成功");
            ImGui::Text("解析模式: %s", (mode == Plugin::DependencyResolutionMode::Strict) ? "严格" : "宽松");
            ImGui::Text("加载顺序: %zu 个插件", result.load_order.size());

            // 显示加载顺序
            if (ImGui::CollapsingHeader("加载顺序详情")) {
                if (ImGui::BeginTable("LoadOrderTable", 3,
                                     ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("顺序");
                    ImGui::TableSetupColumn("插件名称");
                    ImGui::TableSetupColumn("目标状态");
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < result.load_order.size(); i++) {
                        const auto& entry = result.load_order[i];
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%zu", i + 1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", entry.plugin_name.c_str());
                        ImGui::TableSetColumnIndex(2);
                        const char* state_str = "";
                        switch (entry.target_state) {
                            case Plugin::PluginState::Enabled: state_str = "启用"; break;
                            case Plugin::PluginState::Loaded: state_str = "加载"; break;
                            case Plugin::PluginState::Disabled: state_str = "禁用"; break;
                            default: state_str = "未知"; break;
                        }
                        ImGui::Text("%s", state_str);
                    }
                    ImGui::EndTable();
                }
            }

            // 被禁用的插件（宽松模式）
            if (!result.disabled_plugins.empty()) {
                if (ImGui::CollapsingHeader("被禁用的插件")) {
                    for (const auto& name : result.disabled_plugins) {
                        ImGui::Text("- %s (依赖未满足)", name.c_str());
                    }
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "✗ 依赖解析失败");

            // 显示错误列表
            if (!result.errors.empty()) {
                if (ImGui::CollapsingHeader("错误详情")) {
                    for (size_t i = 0; i < result.errors.size(); i++) {
                        const auto& error = result.errors[i];
                        ImGui::TextWrapped("%zu. %s", i + 1, error.to_string().c_str());

                        // 显示循环依赖链
                        if (!error.dependency_chain.empty() &&
                            error.type == Plugin::DependencyErrorType::CircularDependency) {
                            ImGui::Indent();
                            ImGui::TextDisabled("循环依赖链:");
                            for (const auto& name : error.dependency_chain) {
                                ImGui::TextDisabled("  -> %s", name.c_str());
                            }
                            ImGui::Unindent();
                        }
                        ImGui::Spacing();
                    }
                }
            }
        }
    }
}

void PluginManagerWidget::draw_dependency_graph() {
    // 视图模式切换按钮
    static bool showNodeGraph = true;

    ImGui::AlignTextToFramePadding();
    ImGui::Text("视图模式:");
    ImGui::SameLine();

    if (ImGui::RadioButton("节点图", showNodeGraph)) {
        showNodeGraph = true;
    }
    ImGui::SameLine();

    if (ImGui::RadioButton("列表", !showNodeGraph)) {
        showNodeGraph = false;
    }

    ImGui::Separator();

    if (showNodeGraph) {
        // 节点图视图
        if (m_dependencyGraph) {
            // 设置全屏高度用于节点图
            ImVec2 content_avail = ImGui::GetContentRegionAvail();
            m_dependencyGraph->render();
        } else {
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "依赖关系图组件未初始化");
        }
    } else {
        // 列表视图（原有逻辑）
        if (m_selected_plugin < 0 || m_selected_plugin >= static_cast<int>(m_plugins.size())) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "选择一个插件查看依赖关系");
            return;
        }

        const auto& plugin = m_plugins[m_selected_plugin];

        ImGui::Text("%s 的依赖关系", plugin.name.c_str());
        ImGui::Separator();

        // 从 PluginManager 获取插件实例
        auto& pm = Plugin::PluginManager::instance();
        auto* plugin_ptr = pm.get_plugin(plugin.name);

        if (!plugin_ptr) {
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "无法获取插件实例");
            return;
        }

        // 获取依赖列表
        auto dependencies = plugin_ptr->get_dependencies();

        if (dependencies.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "此插件没有依赖");
            return;
        }

        // 显示依赖信息
        ImGui::Text("依赖数量: %zu", dependencies.size());
        ImGui::Spacing();

        for (const auto& dep : dependencies) {
            // 获取依赖插件的状态
            auto dep_plugin = pm.get_plugin(dep.plugin_name);
            const char* status_text = "未知";
            ImVec4 status_color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

            if (dep_plugin) {
                // 依赖存在
                auto state_result = pm.get_plugin_state(dep.plugin_name);
                if (state_result.isOk()) {
                    auto state = state_result.unwrap();
                    if (state == Plugin::PluginState::Enabled) {
                        status_text = "[已启用]";
                        status_color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
                    } else if (state == Plugin::PluginState::Loaded) {
                        status_text = "[已加载]";
                        status_color = ImVec4(0.2f, 0.6f, 0.8f, 1.0f);
                    } else {
                        status_text = "[未就绪]";
                        status_color = ImVec4(0.9f, 0.5f, 0.0f, 1.0f);
                    }
                }
            } else {
                // 依赖不存在
                if (dep.type == Plugin::DependencyType::Required) {
                    status_text = "[缺失-必需]";
                    status_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
                } else if (dep.type == Plugin::DependencyType::Optional) {
                    status_text = "[缺失-可选]";
                    status_color = ImVec4(0.9f, 0.5f, 0.0f, 1.0f);
                } else {
                    status_text = "[缺失-软依赖]";
                    status_color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
                }
            }

            // 绘制依赖类型徽章
            const char* type_badge = "";
            const char* type_badge_color = "";
            if (dep.type == Plugin::DependencyType::Required) {
                type_badge = "[必需]";
                type_badge_color = "\xFF\x55\x55"; // 红色
            } else if (dep.type == Plugin::DependencyType::Optional) {
                type_badge = "[可选]";
                type_badge_color = "\xFF\xAA\x55"; // 橙色
            } else {
                type_badge = "[软]";
                type_badge_color = "\xFF\xFF\xFF"; // 白色
            }

            // 显示依赖项
            ImGui::Text("%s", dep.plugin_name.c_str());
            ImGui::SameLine();
            ImGui::TextColored(status_color, "%s", status_text);
            ImGui::SameLine();
            ImGui::TextDisabled("%s", type_badge);

            // 显示版本要求
            if (dep.version_range.to_string() != "*") {
                ImGui::SameLine();
                ImGui::TextDisabled("(版本: %s)", dep.version_range.to_string().c_str());
            }

            ImGui::Spacing();
        }
    }
}

void PluginManagerWidget::draw_state_badge(const std::string& state) {
    ImVec4 color = get_state_color(state);
    const char* text = get_state_text(state);

    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("[%s]", text);
    ImGui::PopStyleColor();
}

const char* PluginManagerWidget::get_state_text(const std::string& state) {
    if (state == "Enabled") return "已启用";
    if (state == "Disabled") return "已禁用";
    if (state == "Loaded") return "已加载";
    if (state == "Unloaded") return "未加载";
    if (state == "Error") return "错误";
    return "未知";
}

ImVec4 PluginManagerWidget::get_state_color(const std::string& state) {
    if (state == "Enabled") return ImVec4(0.2f, 0.8f, 0.2f, 1.0f);  // 绿色
    if (state == "Disabled") return ImVec4(0.6f, 0.6f, 0.6f, 1.0f); // 灰色
    if (state == "Loaded") return ImVec4(0.2f, 0.6f, 0.8f, 1.0f);  // 蓝色
    if (state == "Unloaded") return ImVec4(0.9f, 0.5f, 0.0f, 1.0f); // 橙色
    if (state == "Error") return ImVec4(0.8f, 0.2f, 0.2f, 1.0f);    // 红色
    return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);                      // 默认灰色
}

// 重载版本：接受 PluginState 枚举
const char* PluginManagerWidget::get_state_text_by_state(Plugin::PluginState state) {
    switch (state) {
        case Plugin::PluginState::Enabled: return "已启用";
        case Plugin::PluginState::Disabled: return "已禁用";
        case Plugin::PluginState::Loaded: return "已加载";
        case Plugin::PluginState::Unloaded: return "未加载";
        case Plugin::PluginState::Error: return "错误";
    }
    return "未知";
}

ImVec4 PluginManagerWidget::get_state_color_by_state(Plugin::PluginState state) {
    switch (state) {
        case Plugin::PluginState::Enabled: return ImVec4(0.2f, 0.8f, 0.2f, 1.0f);  // 绿色
        case Plugin::PluginState::Disabled: return ImVec4(0.6f, 0.6f, 0.6f, 1.0f); // 灰色
        case Plugin::PluginState::Loaded: return ImVec4(0.2f, 0.6f, 0.8f, 1.0f);  // 蓝色
        case Plugin::PluginState::Unloaded: return ImVec4(0.9f, 0.5f, 0.0f, 1.0f); // 橙色
        case Plugin::PluginState::Error: return ImVec4(0.8f, 0.2f, 0.2f, 1.0f);    // 红色
    }
    return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);                      // 默认灰色
}
