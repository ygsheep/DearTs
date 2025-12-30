/**
 * @file theme_settings_widget.hpp
 * @brief 主题设置组件
 * @details 从 SettingsView 中提取的主题设置独立组件
 */

#pragma once

#include <string>
#include <vector>

namespace DearTs::Plugins::Settings {

/**
 * @brief 主题设置组件
 *
 * 负责渲染和管理所有主题相关的设置项：
 * - 字体和窗口设置（字体大小、缩放比例、快速预设）
 * - 主题选择（暗色、亮色、经典）
 * - 主题预览（UI 元素展示）
 */
class ThemeSettingsWidget {
public:
    ThemeSettingsWidget() = default;
    ~ThemeSettingsWidget() = default;

    /**
     * @brief 渲染主题设置面板
     */
    void render();

    /**
     * @brief 获取已修改的配置键列表
     */
    const std::vector<std::string>& get_modified_keys() const {
        return m_modified_keys;
    }

    /**
     * @brief 清空已修改配置键列表
     */
    void clear_modified_keys() {
        m_modified_keys.clear();
    }

private:
    /**
     * @brief 渲染字体和窗口设置
     * @details 字体大小滑块、窗口缩放、快速预设按钮
     */
    void render_font_and_window_settings();

    /**
     * @brief 渲染主题选择区域
     * @details 暗色、亮色、经典主题切换按钮
     */
    void render_theme_selection();

    /**
     * @brief 渲染主题预览区域
     * @details 展示当前主题下的各种 UI 元素效果
     */
    void render_theme_preview();

    /**
     * @brief 标记配置项为已修改
     */
    void mark_modified(const std::string& key);

private:
    std::vector<std::string> m_modified_keys;  ///< 已修改的配置键
    bool m_needs_restart = false;              ///< 是否需要重启应用
    bool m_preview_checkbox = false;           ///< 预览复选框状态
    float m_preview_value = 50.0f;             ///< 预览滑块值
};

} // namespace DearTs::Plugins::Settings
