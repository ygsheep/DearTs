/**
 * @file toast_settings_widget.hpp
 * @brief Toast 设置组件
 * @details 将 SettingsView 中的 Toast 设置拆分为独立组件
 * @author DearTs Team
 * @date 2025-12-30
 */

#pragma once

#include <string>
#include <vector>

namespace DearTs::Plugins::Settings {

/**
 * @brief Toast 设置组件类
 * @details 负责 Toast 通知相关的设置 UI 渲染
 */
class ToastSettingsWidget {
public:
    ToastSettingsWidget() = default;
    ~ToastSettingsWidget() = default;

    /**
     * @brief 渲染完整的 Toast 设置界面
     */
    void render();

    /**
     * @brief 获取修改的配置键列表
     */
    [[nodiscard]] const std::vector<std::string>& get_modified_keys() const {
        return m_modified_keys;
    }

    /**
     * @brief 清空修改记录
     */
    void clear_modified_keys() {
        m_modified_keys.clear();
    }

private:
    /**
     * @brief 渲染动画设置部分
     * @details 包括进入/退出动画时长
     */
    void render_animation_settings();

    /**
     * @brief 渲染布局设置部分
     * @details 包括宽度、内边距、间距、位置
     */
    void render_layout_settings();

    /**
     * @brief 渲染显示设置部分
     * @details 包括最大同时显示数量
     */
    void render_display_settings();

    /**
     * @brief 渲染交互选项部分
     * @details 包括进度条、关闭按钮、悬停暂停、点击关闭
     */
    void render_interaction_settings();

    /**
     * @brief 渲染测试按钮部分
     * @details 提供不同类型 Toast 的测试按钮
     */
    void render_test_buttons();

    /**
     * @brief 标记配置键为已修改
     * @param key 配置键
     */
    void mark_modified(const std::string& key);

private:
    std::vector<std::string> m_modified_keys;
};

} // namespace DearTs::Plugins::Settings
