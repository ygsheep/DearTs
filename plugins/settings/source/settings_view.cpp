/**
 * @file settings_view.cpp
 * @brief 设置视图实现
 */

#include "views/settings_view.hpp"
#include "core/config/config_manager.h"
#include "core/ui/theme_manager.h"
#include "toast_manager.hpp"
#include "liblogger/logger.h"
#include <imgui.h>
#include <algorithm>

namespace DearTs::Plugins::Settings {

SettingsView::SettingsView()
    : ViewWindow("设置") {
}

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
    std::string log_path = config.get_or<std::string>("logger.file_path", "logs/deartsdl_gui.log");
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
    ImGui::Text("主题配置");
    ImGui::Separator();
    ImGui::Spacing();

    auto& theme_manager = Core::UI::ThemeManager::instance();
    auto& config = Core::Config::ConfigManager::instance();
    Core::UI::Theme current_theme = theme_manager.getCurrentTheme();

    // ==================== 字体和窗口设置（放在前面） ====================
    ImGui::Text("字体和窗口设置");
    ImGui::Separator();

    // 字体大小
    float font_size = static_cast<float>(config.get_or<double>("dearts.font.size", 16.0));

    ImGui::Text("字体大小: %.1f px", font_size);
    if (ImGui::SliderFloat("##font_size", &font_size, 12.0f, 24.0f, "%.1f px")) {
        // 字体大小已修改
        config.set("dearts.font.size", static_cast<double>(font_size));
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "dearts.font.size") == m_modified_keys.end()) {
            m_modified_keys.push_back("dearts.font.size");
        }
        m_needs_restart = true;
    }

    // 窗口缩放
    float window_scale = static_cast<float>(config.get_or<double>("dearts.window.scale", 1.0));

    ImGui::Text("窗口缩放: %.0f%%", window_scale * 100.0f);
    if (ImGui::SliderFloat("##window_scale", &window_scale, 0.5f, 2.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
        // 窗口缩放已修改
        config.set("dearts.window.scale", static_cast<double>(window_scale));
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "dearts.window.scale") == m_modified_keys.end()) {
            m_modified_keys.push_back("dearts.window.scale");
        }
        m_needs_restart = true;
    }

    // 预设按钮
    ImGui::Spacing();
    ImGui::Text("快速预设:");

    if (ImGui::Button("小号 (14px)")) {
        config.set("dearts.font.size", 14.0);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "dearts.font.size") == m_modified_keys.end()) {
            m_modified_keys.push_back("dearts.font.size");
        }
        m_needs_restart = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("标准 (16px)")) {
        config.set("dearts.font.size", 16.0);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "dearts.font.size") == m_modified_keys.end()) {
            m_modified_keys.push_back("dearts.font.size");
        }
        m_needs_restart = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("大号 (18px)")) {
        config.set("dearts.font.size", 18.0);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "dearts.font.size") == m_modified_keys.end()) {
            m_modified_keys.push_back("dearts.font.size");
        }
        m_needs_restart = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("超大 (20px)")) {
        config.set("dearts.font.size", 20.0);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "dearts.font.size") == m_modified_keys.end()) {
            m_modified_keys.push_back("dearts.font.size");
        }
        m_needs_restart = true;
    }

    // 重启提示
    ImGui::Spacing();
    if (m_needs_restart) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
        ImGui::TextWrapped("⚠ 字体或缩放已更改，需重启应用生效！");
        ImGui::PopStyleColor();

        if (ImGui::Button("保存并重启")) {
            save_config();
            LOG_INFO("用户请求重启以应用字体/缩放更改");
            ImGui::Text("配置已保存，请重启应用");
        }
    } else {
        ImGui::TextDisabled("ℹ 字体和缩放更改需重启生效");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ==================== 主题选择 ====================
    ImGui::Text("主题选择");
    ImGui::Text("当前主题: %s", Core::UI::ThemeManager::getThemeName(current_theme));
    ImGui::Spacing();

    // 暗色主题
    if (current_theme == Core::UI::Theme::Dark) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    }
    if (ImGui::Button("暗色主题", ImVec2(120, 0))) {
        theme_manager.setTheme(Core::UI::Theme::Dark);
        theme_manager.applyImGuiStyle();
        LOG_INFO("Theme changed to Dark");
    }
    if (current_theme == Core::UI::Theme::Dark) {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();

    // 亮色主题
    if (current_theme == Core::UI::Theme::Light) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    }
    if (ImGui::Button("亮色主题", ImVec2(120, 0))) {
        theme_manager.setTheme(Core::UI::Theme::Light);
        theme_manager.applyImGuiStyle();
        LOG_INFO("Theme changed to Light");
    }
    if (current_theme == Core::UI::Theme::Light) {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();

    // 经典主题
    if (current_theme == Core::UI::Theme::Classic) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    }
    if (ImGui::Button("经典主题", ImVec2(120, 0))) {
        theme_manager.setTheme(Core::UI::Theme::Classic);
        theme_manager.applyImGuiStyle();
        LOG_INFO("Theme changed to Classic");
    }
    if (current_theme == Core::UI::Theme::Classic) {
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 快捷键提示
    ImGui::Text("快捷键:");
    ImGui::BulletText("Ctrl+Alt+1 - 暗色主题");
    ImGui::BulletText("Ctrl+Alt+2 - 亮色主题");

    ImGui::Spacing();

    // 主题预览区域
    ImGui::Text("预览:");
    ImGui::Separator();

    // 显示一些 UI 元素预览当前主题
    ImGui::Text("普通文本");
    ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f), "彩色文本");
    ImGui::Checkbox("示例复选框", &m_show_confirm_reset);
    if (ImGui::Button("示例按钮")) {
        LOG_INFO("Theme preview button clicked");
    }

    // 滑块示例
    static float preview_value = 50.0f;
    ImGui::SliderFloat("示例滑块", &preview_value, 0.0f, 100.0f);

    // 进度条示例
    ImGui::Text("进度条:");
    ImGui::ProgressBar(0.75f, ImVec2(-1, 0), "75%");

    // 颜色编辑器示例
    static ImVec4 preview_color = ImVec4(0.3f, 0.6f, 0.9f, 1.0f);
    ImGui::ColorEdit4("颜色选择器", (float*)&preview_color);
}

void SettingsView::draw_toast_settings() {
    ImGui::Text("气泡消息配置");
    ImGui::Separator();
    ImGui::Spacing();

    auto& config = Core::Config::ConfigManager::instance();

    // Slider 最小/最大值常量（MSVC 不支持复合字面量）
    const double min_duration = 0.1;
    const double max_duration = 1.0;
    const double min_width = 200.0;
    const double max_width_val = 600.0;
    const double min_padding = 10.0;
    const double max_padding_x = 40.0;
    const double max_padding_y = 30.0;
    const double min_spacing = 4.0;
    const double max_spacing = 20.0;

    // ==================== 动画设置 ====================
    ImGui::Text("动画设置");
    ImGui::Separator();

    // 进入动画时长
    double enter_duration = config.get_or<double>("toast_notification.enter_duration", 0.5);
    double original_enter = enter_duration;

    ImGui::Text("进入动画时长: %.2f 秒", enter_duration);
    if (ImGui::SliderScalar("##enter_duration", ImGuiDataType_Double, &enter_duration, &min_duration, &max_duration, "%.2f 秒")) {
        config.set("toast_notification.enter_duration", enter_duration);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.enter_duration") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.enter_duration");
        }
    }

    // 退出动画时长
    double exit_duration = config.get_or<double>("toast_notification.exit_duration", 0.3);
    double original_exit = exit_duration;

    ImGui::Text("退出动画时长: %.2f 秒", exit_duration);
    if (ImGui::SliderScalar("##exit_duration", ImGuiDataType_Double, &exit_duration, &min_duration, &max_duration, "%.2f 秒")) {
        config.set("toast_notification.exit_duration", exit_duration);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.exit_duration") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.exit_duration");
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ==================== 布局设置 ====================
    ImGui::Text("布局设置");
    ImGui::Separator();

    // 最大宽度
    double max_width = config.get_or<double>("toast_notification.max_width", 400.0);
    double original_max_width = max_width;

    ImGui::Text("最大宽度: %.0f px", max_width);
    if (ImGui::SliderScalar("##max_width", ImGuiDataType_Double, &max_width, &min_width, &max_width_val, "%.0f px")) {
        config.set("toast_notification.max_width", max_width);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.max_width") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.max_width");
        }
    }

    // 水平内边距
    double padding_x = config.get_or<double>("toast_notification.padding_x", 20.0);
    double original_padding_x = padding_x;

    ImGui::Text("水平内边距: %.0f px", padding_x);
    if (ImGui::SliderScalar("##padding_x", ImGuiDataType_Double, &padding_x, &min_padding, &max_padding_x, "%.0f px")) {
        config.set("toast_notification.padding_x", padding_x);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.padding_x") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.padding_x");
        }
    }

    // 垂直内边距
    double padding_y = config.get_or<double>("toast_notification.padding_y", 16.0);
    double original_padding_y = padding_y;

    ImGui::Text("垂直内边距: %.0f px", padding_y);
    if (ImGui::SliderScalar("##padding_y", ImGuiDataType_Double, &padding_y, &min_padding, &max_padding_y, "%.0f px")) {
        config.set("toast_notification.padding_y", padding_y);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.padding_y") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.padding_y");
        }
    }

    // Toast 间距
    double spacing = config.get_or<double>("toast_notification.spacing", 8.0);
    double original_spacing = spacing;

    ImGui::Text("Toast 间距: %.0f px", spacing);
    if (ImGui::SliderScalar("##spacing", ImGuiDataType_Double, &spacing, &min_spacing, &max_spacing, "%.0f px")) {
        config.set("toast_notification.spacing", spacing);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.spacing") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.spacing");
        }
    }

    // Toast 位置
    int position = config.get_or<int>("toast_notification.position", 2);  // 默认 TopRight (2)
    int original_position = position;

    ImGui::Text("显示位置:");
    ImGui::Spacing();

    const char* position_items[] = { "左上角", "上中", "右上角", "左下角", "下中", "右下角" };
    if (ImGui::Combo("##position", &position, position_items, 6)) {
        config.set("toast_notification.position", position);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.position") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.position");
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(选择 Toast 显示位置)");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ==================== 显示设置 ====================
    ImGui::Text("显示设置");
    ImGui::Separator();

    // 最大同时显示数量
    int max_toasts = config.get_or<int>("toast_notification.max_toasts", 5);
    int original_max_toasts = max_toasts;

    ImGui::Text("最大同时显示数量: %d", max_toasts);
    if (ImGui::SliderInt("##max_toasts", &max_toasts, 1, 10)) {
        config.set("toast_notification.max_toasts", max_toasts);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.max_toasts") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.max_toasts");
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ==================== 交互选项 ====================
    ImGui::Text("交互选项");
    ImGui::Separator();

    // 显示进度条
    bool show_progress_bar = config.get_or<bool>("toast_notification.show_progress_bar", true);
    bool original_show_progress = show_progress_bar;

    if (ImGui::Checkbox("显示进度条", &show_progress_bar)) {
        config.set("toast_notification.show_progress_bar", show_progress_bar);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.show_progress_bar") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.show_progress_bar");
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(显示剩余时间)");

    // 显示关闭按钮
    bool show_close_button = config.get_or<bool>("toast_notification.show_close_button", true);
    bool original_show_close = show_close_button;

    if (ImGui::Checkbox("显示关闭按钮", &show_close_button)) {
        config.set("toast_notification.show_close_button", show_close_button);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.show_close_button") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.show_close_button");
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(右上角 ✕ 按钮)");

    // 悬停暂停
    bool pause_on_hover = config.get_or<bool>("toast_notification.pause_on_hover", true);
    bool original_pause = pause_on_hover;

    if (ImGui::Checkbox("悬停暂停计时", &pause_on_hover)) {
        config.set("toast_notification.pause_on_hover", pause_on_hover);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.pause_on_hover") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.pause_on_hover");
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(鼠标悬停时暂停倒计时)");

    // 点击关闭
    bool click_to_close = config.get_or<bool>("toast_notification.click_to_close", false);
    bool original_click = click_to_close;

    if (ImGui::Checkbox("点击关闭", &click_to_close)) {
        config.set("toast_notification.click_to_close", click_to_close);
        if (std::find(m_modified_keys.begin(), m_modified_keys.end(), "toast_notification.click_to_close") == m_modified_keys.end()) {
            m_modified_keys.push_back("toast_notification.click_to_close");
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(点击 Toast 窗口关闭)");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ==================== 测试按钮 ====================
    ImGui::Text("测试气泡消息");
    ImGui::Separator();

    ImGui::Text("点击下方按钮预览不同类型的气泡消息：");

    if (ImGui::Button("信息提示")) {
        DearTs::Plugins::Toast::ToastManager::instance().info("信息", "这是一条信息提示");
    }
    ImGui::SameLine();
    if (ImGui::Button("成功提示")) {
        DearTs::Plugins::Toast::ToastManager::instance().success("成功", "操作已成功完成");
    }
    ImGui::SameLine();
    if (ImGui::Button("警告提示")) {
        DearTs::Plugins::Toast::ToastManager::instance().warning("警告", "请注意可能存在的问题");
    }
    ImGui::SameLine();
    if (ImGui::Button("错误提示")) {
        DearTs::Plugins::Toast::ToastManager::instance().error("错误", "操作失败，请重试");
    }

    // 说明文本
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("ℹ 配置更改会实时生效，保存后下次启动仍会保留");
}

void SettingsView::draw_action_buttons() {
    // 所有按钮已移到顶部工具栏
    // 此函数保留以避免编译错误
}

void SettingsView::save_config() {
    auto& config = Core::Config::ConfigManager::instance();

    // 检查是否有 toast 相关的配置被修改
    bool has_toast_changes = false;
    for (const auto& key : m_modified_keys) {
        if (key.find("toast_notification.") == 0) {
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
