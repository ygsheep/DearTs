/**
 * @file theme_settings_widget.cpp
 * @brief 主题设置组件实现
 */

#include "widgets/theme_settings_widget.hpp"
#include "core/config/config_manager.h"
#include "core/ui/theme_manager.h"
#include "liblogger/logger.h"
#include <imgui.h>
#include <algorithm>

namespace DearTs::Plugins::Settings {

void ThemeSettingsWidget::render() {
    ImGui::Text("主题配置");
    ImGui::Separator();
    ImGui::Spacing();

    // 1. 字体和窗口设置
    render_font_and_window_settings();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 2. 主题选择
    render_theme_selection();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 3. 玻璃态样式设置
    render_glassmorphism_settings();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 4. 主题预览
    render_theme_preview();
}

void ThemeSettingsWidget::render_font_and_window_settings() {
    ImGui::Text("字体和窗口设置");
    ImGui::Separator();

    auto& config = Core::Config::ConfigManager::instance();

    // 字体大小
    float font_size = static_cast<float>(config.get_or<double>("dearts.font.size", 16.0));

    ImGui::Text("字体大小: %.1f px", font_size);
    if (ImGui::SliderFloat("##font_size", &font_size, 12.0f, 24.0f, "%.1f px")) {
        config.set("dearts.font.size", static_cast<double>(font_size));
        mark_modified("dearts.font.size");
        m_needs_restart = true;
    }

    // 窗口缩放
    float window_scale = static_cast<float>(config.get_or<double>("dearts.window.scale", 1.0));

    ImGui::Text("窗口缩放: %.0f%%", window_scale * 100.0f);
    if (ImGui::SliderFloat("##window_scale", &window_scale, 0.5f, 2.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
        config.set("dearts.window.scale", static_cast<double>(window_scale));
        mark_modified("dearts.window.scale");
        m_needs_restart = true;
    }

    // 预设按钮
    ImGui::Spacing();
    ImGui::Text("快速预设:");

    if (ImGui::Button("小号 (14px)")) {
        config.set("dearts.font.size", 14.0);
        mark_modified("dearts.font.size");
        m_needs_restart = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("标准 (16px)")) {
        config.set("dearts.font.size", 16.0);
        mark_modified("dearts.font.size");
        m_needs_restart = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("大号 (18px)")) {
        config.set("dearts.font.size", 18.0);
        mark_modified("dearts.font.size");
        m_needs_restart = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("超大 (20px)")) {
        config.set("dearts.font.size", 20.0);
        mark_modified("dearts.font.size");
        m_needs_restart = true;
    }

    // 重启提示
    ImGui::Spacing();
    if (m_needs_restart) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
        ImGui::TextWrapped("⚠ 字体或缩放已更改，需重启应用生效！");
        ImGui::PopStyleColor();

        if (ImGui::Button("保存并重启")) {
            // Note: This is handled by the parent SettingsView's save_config method
            LOG_INFO("用户请求重启以应用字体/缩放更改");
            ImGui::Text("配置已保存，请重启应用");
        }
    } else {
        ImGui::TextDisabled("ℹ 字体和缩放更改需重启生效");
    }
}

void ThemeSettingsWidget::render_theme_selection() {
    auto& theme_manager = Core::UI::ThemeManager::instance();
    Core::UI::Theme current_theme = theme_manager.getCurrentTheme();

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
}

void ThemeSettingsWidget::render_glassmorphism_settings() {
    auto& theme_manager = Core::UI::ThemeManager::instance();

    ImGui::Text("玻璃态样式设置");
    ImGui::Separator();
    ImGui::Spacing();

    // 透明度设置
    float glass_alpha = theme_manager.getGlassAlpha();
    ImGui::Text("玻璃态透明度: %.2f", glass_alpha);
    if (ImGui::SliderFloat("##glass_alpha", &glass_alpha, 0.3f, 1.0f, "%.2f")) {
        theme_manager.setGlassAlpha(glass_alpha);
        theme_manager.applyGlassmorphismStyle();
        LOG_INFO("Glass alpha changed to: {:.2f}", glass_alpha);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("控制玻璃态效果的透明度，值越小越透明");
    }

    // 圆角设置
    float border_radius = theme_manager.getBorderRadius();
    ImGui::Text("圆角半径: %.1f px", border_radius);
    if (ImGui::SliderFloat("##border_radius", &border_radius, 0.0f, 16.0f, "%.1f px")) {
        theme_manager.setBorderRadius(border_radius);
        theme_manager.applyGlassmorphismStyle();
        LOG_INFO("Border radius changed to: {:.1f}", border_radius);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("控制 UI 元素的圆角半径，0 为直角");
    }

    // 强调色设置
    ImVec4 accent_color = theme_manager.getAccentColor();
    ImGui::Text("强调色:");
    if (ImGui::ColorEdit4("##accent_color", reinterpret_cast<float*>(&accent_color),
                          ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar)) {
        theme_manager.setAccentColor(accent_color);
        theme_manager.applyGlassmorphismStyle();
        LOG_INFO("Accent color changed to: ({:.2f}, {:.2f}, {:.2f}, {:.2f})",
                 accent_color.x, accent_color.y, accent_color.z, accent_color.w);
    }

    // 预设颜色
    ImGui::Spacing();
    ImGui::Text("预设颜色:");

    struct PresetColor {
        const char* name;
        ImVec4 color;
    };

    static PresetColor presets[] = {
        {"蓝色", {0.3f, 0.6f, 1.0f, 1.0f}},
        {"紫色", {0.6f, 0.3f, 1.0f, 1.0f}},
        {"绿色", {0.3f, 0.9f, 0.5f, 1.0f}},
        {"橙色", {1.0f, 0.6f, 0.2f, 1.0f}},
        {"红色", {1.0f, 0.3f, 0.3f, 1.0f}},
        {"青色", {0.2f, 0.8f, 0.9f, 1.0f}},
    };

    for (const auto& preset : presets) {
        ImGui::PushStyleColor(ImGuiCol_Button, preset.color);
        if (ImGui::Button(preset.name, ImVec2(70, 0))) {
            theme_manager.setAccentColor(preset.color);
            theme_manager.applyGlassmorphismStyle();
            LOG_INFO("Accent color preset selected: {}", preset.name);
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
    }
    ImGui::NewLine();

    // 应用按钮
    ImGui::Spacing();
    if (ImGui::Button("应用玻璃态样式", ImVec2(160, 0))) {
        theme_manager.applyGlassmorphismStyle();
        LOG_INFO("Glassmorphism style applied manually");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("更改会自动应用");
}

void ThemeSettingsWidget::render_theme_preview() {
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
    ImGui::Checkbox("示例复选框", &m_preview_checkbox);
    if (ImGui::Button("示例按钮")) {
        LOG_INFO("Theme preview button clicked");
    }

    // 滑块示例
    ImGui::SliderFloat("示例滑块", &m_preview_value, 0.0f, 100.0f);

    // 进度条示例
    ImGui::Text("进度条:");
    ImGui::ProgressBar(0.75f, ImVec2(-1, 0), "75%");

    // 颜色编辑器示例
    static ImVec4 preview_color = ImVec4(0.3f, 0.6f, 0.9f, 1.0f);
    ImGui::ColorEdit4("颜色选择器", reinterpret_cast<float*>(&preview_color));
}

void ThemeSettingsWidget::mark_modified(const std::string& key) {
    if (std::find(m_modified_keys.begin(), m_modified_keys.end(), key) == m_modified_keys.end()) {
        m_modified_keys.push_back(key);
    }
}

} // namespace DearTs::Plugins::Settings
