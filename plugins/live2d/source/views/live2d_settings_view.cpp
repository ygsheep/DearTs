/**
 * @file live2d_settings_view.cpp
 * @brief Live2D 设置视图实现
 */

#include "views/live2d_settings_view.hpp"
#include "live2d_plugin.hpp"
#include "core/config/config_manager.h"
#include "core/ui/icon_font.hpp"
#include "liblogger/logger.h"
#include <imgui.h>

namespace DearTs::Plugins::Live2D {

Live2DSettingsView::Live2DSettingsView(Live2DPlugin* plugin)
    : ViewWindow("Live2D 设置", ICON_SETTINGS)
    , m_plugin(plugin)
    , m_widget(std::make_unique<Live2DSettingsWidget>(plugin))
{
}

void Live2DSettingsView::draw_content() {
    // 顶部工具栏
    draw_toolbar();

    ImGui::Separator();
    ImGui::Spacing();

    // 设置内容
    m_widget->render();

    ImGui::Spacing();
    ImGui::Separator();

    // 底部状态栏
    if (m_widget->has_modifications()) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f),
                          "有 %zu 项配置未保存",
                          m_widget->get_modification_count());
    } else {
        ImGui::TextDisabled("所有配置已保存");
    }
}

void Live2DSettingsView::draw_toolbar() {
    // 保存按钮
    bool has_mods = m_widget->has_modifications();
    if (has_mods) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
        if (ImGui::Button("保存配置")) {
            save_config();
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::Button("保存配置");
        ImGui::PopItemFlag();
        ImGui::PopStyleColor(2);
    }

    ImGui::SameLine();

    // 重新加载按钮
    if (ImGui::Button("重新加载")) {
        reload_config();
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    // 活动模型信息
    const auto* active_model = m_plugin->get_active_model();
    if (active_model) {
        const auto& info = active_model->get_info();
        ImGui::Text("活动模型: %s (%dx%d)",
                   info.model_name.c_str(),
                   info.width,
                   info.height);
    } else {
        ImGui::TextDisabled("活动模型: 未加载");
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    // 模型数量
    size_t model_count = m_plugin->get_model_names().size();
    ImGui::Text("已加载: %zu 个模型", model_count);
}

void Live2DSettingsView::save_config() {
    auto& config_manager = DearTs::Core::Config::ConfigManager::instance();
    auto& plugin_config = m_plugin->get_config();

    // 合并所有修改的配置到 ConfigManager
    for (const auto& key : m_widget->get_modified_keys()) {
        // 配置已经在 widget 中实时更新了
        // 这里只需要保存到文件
    }

    // 保存到文件
    auto result = config_manager.save_to_file("config.json");
    if (result.isErr()) {
        LOG_ERROR("保存 Live2D 配置失败: {}", result.error());
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                          "保存失败: %s", result.error().c_str());
    } else {
        LOG_INFO("Live2D 配置已保存");
        m_widget->clear_modified_keys();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "保存成功！");
    }
}

void Live2DSettingsView::reload_config() {
    auto& config_manager = DearTs::Core::Config::ConfigManager::instance();
    auto& plugin_config = m_plugin->get_config();

    // 从文件重新加载
    auto result = config_manager.load_from_file("config.json");
    if (result.isErr()) {
        LOG_WARN("重新加载 Live2D 配置失败: {}", result.error());
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f),
                          "重新加载失败: %s", result.error().c_str());
    } else {
        // 重新加载插件配置
        plugin_config.models_directory = config_manager.get_or<std::string>(
            "live2d.models_directory", "resources/live2d");
        plugin_config.active_model = config_manager.get_or<std::string>(
            "live2d.active_model", "");
        plugin_config.model_scale = static_cast<float>(
            config_manager.get_or<double>("live2d.model_scale", 1.0));
        // ... 其他配置项

        LOG_INFO("Live2D 配置已重新加载");
        m_widget->clear_modified_keys();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "重新加载成功！");
    }
}

} // namespace DearTs::Plugins::Live2D
