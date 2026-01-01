/**
 * @file settings_view.cpp
 * @brief 设置视图实现
 */

#include "views/settings_view.hpp"
#include "toast_settings_widget.hpp"
#include "widgets/theme_settings_widget.hpp"
#include "widgets/character_settings_widget.hpp"
#include "core/config/config_manager.h"
#include "core/ui/theme_manager.h"
#include "core/ui/icon_font.hpp"
#include "core/content/registry_base.h"
#include "toast_manager.hpp"
#include "liblogger/logger.h"
#include <imgui.h>
#include <algorithm>

namespace DearTs::Plugins::Settings {
using DearTs::Core::ContentRegistry::UnlocalizedString;

SettingsView::SettingsView()
    : ViewWindow(UnlocalizedString("设置"), ICON_SETTINGS)
    , m_toast_widget(std::make_unique<ToastSettingsWidget>())
    , m_theme_widget(std::make_unique<ThemeSettingsWidget>())
    , m_character_widget(std::make_unique<CharacterSettingsWidget>()) {
}

SettingsView::~SettingsView() = default;

void SettingsView::draw_content() {
    // 顶部工具栏
    if (ImGui::Button("刷新")) {
        refresh_config_items();
    }
    ImGui::SameLine();

    // 保存更改按钮（如果配置已修改）
    if (!m_modified_keys.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
        if (ImGui::Button("保存更改")) {
            save_config();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::Text("已修改 %zu 项", m_modified_keys.size());
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::Button("保存更改");
        ImGui::PopStyleColor(2);
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::SameLine();
        ImGui::Text("无更改");
        ImGui::PopItemFlag();
    }

    ImGui::SameLine();

    // 重新加载按钮
    if (ImGui::Button("重新加载")) {
        reload_config();
    }

    ImGui::SameLine();

    // 重置默认按钮
    if (ImGui::Button("重置默认")) {
        ImGui::OpenPopup("确认重置");
    }

    ImGui::SameLine();
    ImGui::Text("配置文件: config.json");

    ImGui::Separator();
    ImGui::Spacing();

    // 主布局：侧边栏 + 配置面板
    // 侧边栏宽度
    const float sidebar_width = 150.0f;

    // 开始侧边栏
    ImGui::BeginChild("##SettingsSidebar", ImVec2(sidebar_width, 0), true,
                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    draw_sidebar();

    ImGui::EndChild();

    ImGui::SameLine();

    // 配置内容区域
    ImGui::BeginChild("##SettingsPanel", ImVec2(0, 0), true,
                      ImGuiWindowFlags_NoTitleBar);

    draw_config_panel();

    ImGui::EndChild();

    // 确认重置对话框
    if (ImGui::BeginPopupModal("确认重置", &m_show_confirm_reset, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("确定要将所有配置重置为默认值吗？");
        ImGui::Text("此操作不可撤销。");

        ImGui::Spacing();

        if (ImGui::Button("确定", ImVec2(120, 0))) {
            reset_to_defaults();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SettingsView::draw_sidebar() {
    ImGui::TextUnformatted("配置分类");

    ImGui::Spacing();

    // 绘制分类按钮
    const auto categories = {
        ConfigCategory::General,
        ConfigCategory::Logger,
        ConfigCategory::Window,
        ConfigCategory::Theme,
        ConfigCategory::Character,
        ConfigCategory::Toast,
        ConfigCategory::Shortcuts,
        ConfigCategory::Advanced,
    };

    for (auto category : categories) {
        bool is_selected = (m_current_category == category);
        const char* name = get_category_name(category);

        if (ImGui::Selectable(name, is_selected)) {
            m_current_category = category;
        }
    }
}

void SettingsView::draw_config_panel() {
    const char* category_name = get_category_name(m_current_category);

    ImGui::Text("%s 设置", category_name);
    ImGui::Separator();

    // 根据当前分类绘制对应的设置
    switch (m_current_category) {
        case ConfigCategory::General:
            draw_general_settings();
            break;
        case ConfigCategory::Logger:
            draw_logger_settings();
            break;
        case ConfigCategory::Window:
            draw_window_settings();
            break;
        case ConfigCategory::Theme:
            draw_theme_settings();
            break;
        case ConfigCategory::Character:
            draw_character_settings();
            break;
        case ConfigCategory::Toast:
            draw_toast_settings();
            break;
        default:
            ImGui::Text("此分类暂无设置项");
            break;
    }
}

void SettingsView::draw_config_item(const std::string& key) {
    auto& config = Core::Config::ConfigManager::instance();

    // 获取配置值
    auto result = config.get<std::string>(key);
    if (result.isErr()) {
        ImGui::Text("无法读取配置: %s", key.c_str());
        return;
    }

    std::string value = result.unwrap();

    // 绘制配置项
    ImGui::Text("%s", key.c_str());
    ImGui::SameLine(150);

    // 简单的文本输入（可以根据类型使用不同控件）
    char buffer[256];
    strncpy(buffer, value.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    if (ImGui::InputText(("##" + key).c_str(), buffer, sizeof(buffer))) {
        // 标记为已修改
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), key) == m_modified_keys.end()) {
            m_modified_keys.push_back(key);
        }
    }

    // 如果已修改，显示标记
    if (std::find(m_modified_keys.begin(), m_modified_keys.end(), key) != m_modified_keys.end()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "*");
    }
}

void SettingsView::draw_general_settings() {
    ImGui::Text("通用配置项将在此显示");
    ImGui::Text("（待实现）");

    // 示例：显示一些通用配置
    draw_config_item("dearts.titlebar.frame_padding_x");
    draw_config_item("dearts.titlebar.frame_padding_y");
}

void SettingsView::draw_logger_settings() {
    ImGui::Text("日志配置");

    auto& config = Core::Config::ConfigManager::instance();

    // 日志级别
    int log_level = config.get_or<int>("logger.level", 1);
    int original_level = log_level;

    ImGui::Text("日志级别:");
    ImGui::RadioButton("TRACE (0)", &log_level, 0);
    ImGui::RadioButton("DEBUG (1)", &log_level, 1);
    ImGui::RadioButton("INFO (2)", &log_level, 2);
    ImGui::RadioButton("WARN (3)", &log_level, 3);
    ImGui::RadioButton("ERROR (4)", &log_level, 4);
    ImGui::RadioButton("FATAL (5)", &log_level, 5);

    if (log_level != original_level) {
        config.set("logger.level", log_level);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "logger.level") == m_modified_keys.end()) {
            m_modified_keys.push_back("logger.level");
        }
    }

    ImGui::Spacing();

    // 文件输出开关
    bool file_enabled = config.get_or<bool>("logger.file_enabled", true);
    bool original_enabled = file_enabled;

    ImGui::Checkbox("启用文件输出", &file_enabled);

    if (file_enabled != original_enabled) {
        config.set("logger.file_enabled", file_enabled);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "logger.file_enabled") == m_modified_keys.end()) {
            m_modified_keys.push_back("logger.file_enabled");
        }
    }

    ImGui::Spacing();

    // 日志文件路径
    std::string log_path = config.get_or<std::string>("logger.file_path", "logs/app.log");
    char path_buffer[256];
    strncpy(path_buffer, log_path.c_str(), sizeof(path_buffer));
    path_buffer[sizeof(path_buffer) - 1] = '\0';

    ImGui::Text("日志文件路径:");
    if (ImGui::InputText("##log_path", path_buffer, sizeof(path_buffer))) {
        config.set("logger.file_path", std::string(path_buffer));
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "logger.file_path") == m_modified_keys.end()) {
            m_modified_keys.push_back("logger.file_path");
        }
    }

    // 显示日志级别说明
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("日志级别说明:");
    ImGui::BulletText("TRACE - 最详细的跟踪信息");
    ImGui::BulletText("DEBUG - 调试信息（默认）");
    ImGui::BulletText("INFO - 一般信息");
    ImGui::BulletText("WARN - 警告信息");
    ImGui::BulletText("ERROR - 错误信息");
    ImGui::BulletText("FATAL - 致命错误");
}

void SettingsView::draw_window_settings() {
    ImGui::Text("窗口配置");
    ImGui::Text("（待实现）");
}

void SettingsView::draw_theme_settings() {
    // 使用 ThemeSettingsWidget 组件渲染
    m_theme_widget->render();
}

void SettingsView::draw_character_settings() {
    // 使用 CharacterSettingsWidget 组件渲染
    m_character_widget->render();
}

void SettingsView::draw_toast_settings() {
    // 使用 ToastSettingsWidget 组件渲染
    m_toast_widget->render();
}

void SettingsView::draw_action_buttons() {
    // 所有按钮已移到顶部工具栏
    // 此函数保留以避免编译错误
}

void SettingsView::save_config() {
    auto& config = Core::Config::ConfigManager::instance();

    // 合并 ToastSettingsWidget 的修改列表
    const auto& toast_modified = m_toast_widget->get_modified_keys();
    for (const auto& key : toast_modified) {
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), key) == m_modified_keys.end()) {
            m_modified_keys.push_back(key);
        }
    }

    // 合并 ThemeSettingsWidget 的修改列表
    const auto& theme_modified = m_theme_widget->get_modified_keys();
    for (const auto& key : theme_modified) {
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), key) == m_modified_keys.end()) {
            m_modified_keys.push_back(key);
        }
    }

    // 合并 CharacterSettingsWidget 的修改列表
    const auto& character_modified = m_character_widget->get_modified_keys();
    for (const auto& key : character_modified) {
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), key) == m_modified_keys.end()) {
            m_modified_keys.push_back(key);
        }
    }

    // 检查是否有 toast 相关的配置被修改
    bool has_toast_changes = false;
    for (const auto& key : m_modified_keys) {
        if (key.starts_with("toast_notification.")) {
            has_toast_changes = true;
            break;
        }
    }

    // 如果有 toast 配置更改，实时更新 ToastManager
    if (has_toast_changes) {
        auto& toast_config = DearTs::Plugins::Toast::ToastManager::instance().get_config();

        // 更新动画设置
        toast_config.enter_duration = config.get_or<double>("toast_notification.enter_duration", 0.5);
        toast_config.exit_duration = config.get_or<double>("toast_notification.exit_duration", 0.3);

        // 更新布局设置
        toast_config.max_width = config.get_or<double>("toast_notification.max_width", 400.0);
        toast_config.padding_x = config.get_or<double>("toast_notification.padding_x", 20.0);
        toast_config.padding_y = config.get_or<double>("toast_notification.padding_y", 16.0);
        toast_config.spacing = config.get_or<double>("toast_notification.spacing", 8.0);
        toast_config.position = config.get_or<int>("toast_notification.position", 2);

        // 更新显示设置
        toast_config.max_toasts = config.get_or<int>("toast_notification.max_toasts", 5);

        // 更新交互选项
        toast_config.show_progress_bar = config.get_or<bool>("toast_notification.show_progress_bar", true);
        toast_config.show_close_button = config.get_or<bool>("toast_notification.show_close_button", true);
        toast_config.pause_on_hover = config.get_or<bool>("toast_notification.pause_on_hover", true);
        toast_config.click_to_close = config.get_or<bool>("toast_notification.click_to_close", false);

        LOG_INFO("Toast 配置已实时更新");
    }

    // 保存到文件
    auto result = config.save_to_file("config.json");
    if (result.isErr()) {
        LOG_ERROR("保存配置失败: {}", result.error());
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "保存失败: %s", result.error().c_str());
    } else {
        LOG_INFO("配置已保存");
        m_modified_keys.clear();
        m_toast_widget->clear_modified_keys();  // 清空 toast widget 的修改记录
        m_theme_widget->clear_modified_keys();  // 清空 theme widget 的修改记录
        m_character_widget->clear_modified_keys();  // 清空 character widget 的修改记录
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "保存成功！");
    }
}

void SettingsView::reload_config() {
    auto& config = Core::Config::ConfigManager::instance();

    // 从文件重新加载
    auto result = config.load_from_file("config.json");
    if (result.isErr()) {
        LOG_WARN("重新加载配置失败: {}", result.error());
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "重新加载失败: %s", result.error().c_str());
    } else {
        LOG_INFO("配置已重新加载");
        m_modified_keys.clear();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "重新加载成功！");
    }
}

void SettingsView::reset_to_defaults() {
    auto& config = Core::Config::ConfigManager::instance();

    // 清空所有配置（恢复为默认值）
    config.clear();

    LOG_INFO("配置已重置为默认值");
    m_modified_keys.clear();
}

void SettingsView::refresh_config_items() {
    LOG_INFO("刷新配置项列表");
    m_modified_keys.clear();
}

const char* SettingsView::get_category_name(ConfigCategory category) const {
    switch (category) {
        case ConfigCategory::General:   return "通用";
        case ConfigCategory::Logger:    return "日志";
        case ConfigCategory::Window:    return "窗口";
        case ConfigCategory::Theme:     return "主题";
        case ConfigCategory::Character: return "角色";
        case ConfigCategory::Toast:     return "气泡消息";
        case ConfigCategory::Shortcuts: return "快捷键";
        case ConfigCategory::Advanced:  return "高级";
        default:                        return "未知";
    }
}

std::vector<std::string> SettingsView::filter_configs_by_category(ConfigCategory category) {
    // TODO: 实现配置过滤逻辑
    (void)category;
    return {};
}

} // namespace DearTs::Plugins::Settings
