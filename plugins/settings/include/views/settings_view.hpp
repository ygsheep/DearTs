/**
 * @file settings_view.hpp
 * @brief 设置视图 - 管理 ConfigManager 和系统配置
 * @details 提供图形界面来查看和修改应用程序配置
 */

#pragma once

#include "core/ui/view.h"
#include <string>
#include <vector>

// 前向声明
namespace DearTs::Plugins::Settings {
class ToastSettingsWidget;
class ThemeSettingsWidget;
class CharacterSettingsWidget;
}

namespace DearTs::Plugins::Settings {

/**
 * @brief 配置分类
 */
enum class ConfigCategory {
    General,        ///< 通用设置
    Logger,         ///< 日志设置
    Window,         ///< 窗口设置
    Theme,          ///< 主题设置
    Character,      ///< 角色设置
    Shortcuts,      ///< 快捷键设置
    Toast,          ///< 气泡消息设置
    Advanced,       ///< 高级设置
};

/**
 * @brief 配置项信息
 */
struct ConfigItemInfo {
    std::string key;                    ///< 配置键
    std::string display_name;           ///< 显示名称
    std::string description;            ///< 描述
    ConfigCategory category;            ///< 分类
    bool is_modified;                   ///< 是否已修改
};

/**
 * @brief 设置视图
 *
 * 提供图形界面来管理 ConfigManager 中的所有配置项
 */
class SettingsView : public Core::UI::ViewWindow {
public:
    explicit SettingsView();
    ~SettingsView() override;

    /**
     * @brief 绘制视图内容
     */
    void draw_content() override;

    /**
     * @brief 获取最小窗口大小
     */
    ImVec2 get_min_size() const override {
        return ImVec2(600, 400);
    }

private:
    /**
     * @brief 绘制侧边栏（配置分类）
     */
    void draw_sidebar();

    /**
     * @brief 绘制配置内容区域
     */
    void draw_config_panel();

    /**
     * @brief 绘制单个配置项
     */
    void draw_config_item(const std::string& key);

    /**
     * @brief 绘制通用设置
     */
    void draw_general_settings();

    /**
     * @brief 绘制日志设置
     */
    void draw_logger_settings();

    /**
     * @brief 绘制窗口设置
     */
    void draw_window_settings();

    /**
     * @brief 绘制主题设置
     */
    void draw_theme_settings();

    /**
     * @brief 绘制角色设置
     */
    void draw_character_settings();

    /**
     * @brief 绘制气泡消息设置
     */
    void draw_toast_settings();

    /**
     * @brief 绘制底部按钮栏
     */
    void draw_action_buttons();

    /**
     * @brief 保存配置
     */
    void save_config();

    /**
     * @brief 重新加载配置
     */
    void reload_config();

    /**
     * @brief 重置为默认值
     */
    void reset_to_defaults();

    /**
     * @brief 刷新配置项列表
     */
    void refresh_config_items();

    /**
     * @brief 获取分类名称
     */
    const char* get_category_name(ConfigCategory category) const;

    /**
     * @brief 过滤配置项
     */
    std::vector<std::string> filter_configs_by_category(ConfigCategory category);

private:
    ConfigCategory m_current_category = ConfigCategory::General;
    std::vector<std::string> m_modified_keys;
    bool m_show_confirm_reset = false;
    bool m_search_filter = false;
    char m_search_buffer[256] = "";

    // 字体和缩放设置
    float m_font_size = 16.0f;           ///< 字体大小（像素）
    float m_window_scale = 1.0f;         ///< 窗口缩放比例
    bool m_needs_restart = false;        ///< 是否需要重启应用
    bool m_font_changed = false;         ///< 字体是否已更改

    // 设置组件
    std::unique_ptr<ToastSettingsWidget> m_toast_widget;    ///< Toast 设置组件
    std::unique_ptr<ThemeSettingsWidget> m_theme_widget;    ///< 主题设置组件
    std::unique_ptr<CharacterSettingsWidget> m_character_widget;  ///< 角色设置组件
};

} // namespace DearTs::Plugins::Settings
