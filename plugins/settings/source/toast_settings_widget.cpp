/**
 * @file toast_settings_widget.cpp
 * @brief Toast 设置组件实现
 */

#include "toast_settings_widget.hpp"

#include "core/config/config_manager.h"
#include "plugins/toast_notification/include/toast_manager.hpp"
#include <imgui.h>

namespace DearTs::Plugins::Settings {

void ToastSettingsWidget::render() {
    ImGui::Text("气泡消息配置");
    ImGui::Separator();
    ImGui::Spacing();

    // 渲染各个设置分组
    render_animation_settings();
    render_layout_settings();
    render_display_settings();
    render_interaction_settings();
    render_test_buttons();

    // 说明文本
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("ℹ 配置更改会实时生效，保存后下次启动仍会保留");
}

void ToastSettingsWidget::render_animation_settings() {
    auto& config = Core::Config::ConfigManager::instance();
    auto& toast_config = DearTs::Plugins::Toast::ToastManager::instance().get_config();

    ImGui::Text("动画设置");
    ImGui::Separator();

    // Slider 范围常量
    const double min_duration = 0.1;
    const double max_duration = 1.0;

    // 进入动画时长
    double enter_duration = config.get_or<double>("toast_notification.enter_duration", 0.5);
    ImGui::Text("进入动画时长: %.2f 秒", enter_duration);
    if (ImGui::SliderScalar("##enter_duration", ImGuiDataType_Double, &enter_duration, &min_duration, &max_duration, "%.2f 秒")) {
        config.set("toast_notification.enter_duration", enter_duration);
        toast_config.enter_duration = enter_duration;  // 实时更新 ToastManager
        mark_modified("toast_notification.enter_duration");
    }

    // 退出动画时长
    double exit_duration = config.get_or<double>("toast_notification.exit_duration", 0.3);
    ImGui::Text("退出动画时长: %.2f 秒", exit_duration);
    if (ImGui::SliderScalar("##exit_duration", ImGuiDataType_Double, &exit_duration, &min_duration, &max_duration, "%.2f 秒")) {
        config.set("toast_notification.exit_duration", exit_duration);
        toast_config.exit_duration = exit_duration;  // 实时更新 ToastManager
        mark_modified("toast_notification.exit_duration");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void ToastSettingsWidget::render_layout_settings() {
    auto& config = Core::Config::ConfigManager::instance();
    auto& toast_config = DearTs::Plugins::Toast::ToastManager::instance().get_config();

    ImGui::Text("布局设置");
    ImGui::Separator();

    // Slider 范围常量
    const double min_width = 200.0;
    const double max_width = 600.0;
    const double min_padding = 10.0;
    const double max_padding_x = 40.0;
    const double max_padding_y = 30.0;
    const double min_spacing = 4.0;
    const double max_spacing = 20.0;

    // 最大宽度
    double max_width_val = config.get_or<double>("toast_notification.max_width", 400.0);
    ImGui::Text("最大宽度: %.0f px", max_width_val);
    if (ImGui::SliderScalar("##max_width", ImGuiDataType_Double, &max_width_val, &min_width, &max_width, "%.0f px")) {
        config.set("toast_notification.max_width", max_width_val);
        toast_config.max_width = max_width_val;  // 实时更新 ToastManager
        mark_modified("toast_notification.max_width");
    }

    // 水平内边距
    double padding_x = config.get_or<double>("toast_notification.padding_x", 20.0);
    ImGui::Text("水平内边距: %.0f px", padding_x);
    if (ImGui::SliderScalar("##padding_x", ImGuiDataType_Double, &padding_x, &min_padding, &max_padding_x, "%.0f px")) {
        config.set("toast_notification.padding_x", padding_x);
        toast_config.padding_x = padding_x;  // 实时更新 ToastManager
        mark_modified("toast_notification.padding_x");
    }

    // 垂直内边距
    double padding_y = config.get_or<double>("toast_notification.padding_y", 16.0);
    ImGui::Text("垂直内边距: %.0f px", padding_y);
    if (ImGui::SliderScalar("##padding_y", ImGuiDataType_Double, &padding_y, &min_padding, &max_padding_y, "%.0f px")) {
        config.set("toast_notification.padding_y", padding_y);
        toast_config.padding_y = padding_y;  // 实时更新 ToastManager
        mark_modified("toast_notification.padding_y");
    }

    // Toast 间距
    double spacing = config.get_or<double>("toast_notification.spacing", 8.0);
    ImGui::Text("Toast 间距: %.0f px", spacing);
    if (ImGui::SliderScalar("##spacing", ImGuiDataType_Double, &spacing, &min_spacing, &max_spacing, "%.0f px")) {
        config.set("toast_notification.spacing", spacing);
        toast_config.spacing = spacing;  // 实时更新 ToastManager
        mark_modified("toast_notification.spacing");
    }

    // Toast 位置
    int position = config.get_or<int>("toast_notification.position", 2);  // 默认 TopRight (2)
    ImGui::Text("显示位置:");
    ImGui::Spacing();

    const char* position_items[] = { "左上角", "上中", "右上角", "左下角", "下中", "右下角" };
    if (ImGui::Combo("##position", &position, position_items, 6)) {
        config.set("toast_notification.position", position);
        toast_config.position = position;  // 实时更新 ToastManager
        mark_modified("toast_notification.position");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(选择 Toast 显示位置)");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void ToastSettingsWidget::render_display_settings() {
    auto& config = Core::Config::ConfigManager::instance();
    auto& toast_config = DearTs::Plugins::Toast::ToastManager::instance().get_config();

    ImGui::Text("显示设置");
    ImGui::Separator();

    // 最大同时显示数量
    int max_toasts = config.get_or<int>("toast_notification.max_toasts", 5);
    ImGui::Text("最大同时显示数量: %d", max_toasts);
    if (ImGui::SliderInt("##max_toasts", &max_toasts, 1, 10)) {
        config.set("toast_notification.max_toasts", max_toasts);
        toast_config.max_toasts = max_toasts;  // 实时更新 ToastManager
        mark_modified("toast_notification.max_toasts");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void ToastSettingsWidget::render_interaction_settings() {
    auto& config = Core::Config::ConfigManager::instance();
    auto& toast_config = DearTs::Plugins::Toast::ToastManager::instance().get_config();

    ImGui::Text("交互选项");
    ImGui::Separator();

    // 显示进度条
    bool show_progress_bar = config.get_or<bool>("toast_notification.show_progress_bar", true);
    if (ImGui::Checkbox("显示进度条", &show_progress_bar)) {
        config.set("toast_notification.show_progress_bar", show_progress_bar);
        toast_config.show_progress_bar = show_progress_bar;  // 实时更新 ToastManager
        mark_modified("toast_notification.show_progress_bar");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(显示剩余时间)");

    // 显示关闭按钮
    bool show_close_button = config.get_or<bool>("toast_notification.show_close_button", true);
    if (ImGui::Checkbox("显示关闭按钮", &show_close_button)) {
        config.set("toast_notification.show_close_button", show_close_button);
        toast_config.show_close_button = show_close_button;  // 实时更新 ToastManager
        mark_modified("toast_notification.show_close_button");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(右上角 ✕ 按钮)");

    // 悬停暂停
    bool pause_on_hover = config.get_or<bool>("toast_notification.pause_on_hover", true);
    if (ImGui::Checkbox("悬停暂停计时", &pause_on_hover)) {
        config.set("toast_notification.pause_on_hover", pause_on_hover);
        toast_config.pause_on_hover = pause_on_hover;  // 实时更新 ToastManager
        mark_modified("toast_notification.pause_on_hover");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(鼠标悬停时暂停倒计时)");

    // 点击关闭
    bool click_to_close = config.get_or<bool>("toast_notification.click_to_close", false);
    if (ImGui::Checkbox("点击关闭", &click_to_close)) {
        config.set("toast_notification.click_to_close", click_to_close);
        toast_config.click_to_close = click_to_close;  // 实时更新 ToastManager
        mark_modified("toast_notification.click_to_close");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(点击 Toast 窗口关闭)");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void ToastSettingsWidget::render_test_buttons() {
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
}

void ToastSettingsWidget::mark_modified(const std::string& key) {
    if (std::find(m_modified_keys.begin(), m_modified_keys.end(), key) == m_modified_keys.end()) {
        m_modified_keys.push_back(key);
    }
}

} // namespace DearTs::Plugins::Settings
