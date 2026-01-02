/**
 * @file theme_manager.h
 * @brief 主题管理器
 * @details 管理应用程序主题，支持暗色、亮色和自定义主题
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include <imgui.h>
#include <string>
#include <unordered_map>
#include <functional>

namespace DearTs::Core::UI {

/**
 * @brief 主题类型枚举
 */
enum class Theme {
    Dark,      ///< 暗色主题
    Light,     ///< 亮色主题
    Classic,   ///< 经典主题
    Custom     ///< 自定义主题
};

/**
 * @brief 主题管理器
 *
 * 提供主题管理功能：
 * - 预定义主题切换
 * - 自定义主题加载
 * - ImGui 样式自动应用
 * - 颜色查询和修改
 */
class ThemeManager final {  // 单例类，禁止继承
public:
    /**
     * @brief 获取单例实例（线程安全，Magic Statics）
     */
    static ThemeManager& instance() noexcept {
        static ThemeManager inst;
        return inst;
    }

    // 删除所有拷贝和移动操作
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    ThemeManager(ThemeManager&&) = delete;
    ThemeManager& operator=(ThemeManager&&) = delete;

    /**
     * @brief 设置主题
     * @param theme 主题类型
     */
    void setTheme(Theme theme);

    /**
     * @brief 获取当前主题类型
     */
    [[nodiscard]] Theme getCurrentTheme() const { return m_current_theme; }

    /**
     * @brief 应用 ImGui 样式
     * @details 根据当前主题设置 ImGui 的样式
     */
    void applyImGuiStyle();

    /**
     * @brief 从 JSON 加载自定义主题
     * @param json_path JSON 文件路径
     * @return 成功返回 true
     */
    bool loadTheme(const std::string& json_path);

    /**
     * @brief 保存当前主题到 JSON
     * @param json_path JSON 文件路径
     * @return 成功返回 true
     */
    bool saveTheme(const std::string& json_path) const;

    /**
     * @brief 获取颜色
     * @param key 颜色键名
     * @return 颜色值，如果不存在返回默认白色
     */
    [[nodiscard]] ImVec4 getColor(const std::string& key) const;

    /**
     * @brief 设置颜色
     * @param key 颜色键名
     * @param color 颜色值
     */
    void setColor(const std::string& key, const ImVec4& color);

    /**
     * @brief 获取主题名称
     * @param theme 主题类型
     * @return 主题名称
     */
    [[nodiscard]] static const char* getThemeName(Theme theme);

    /**
     * @brief 注册主题变更回调
     * @param callback 回调函数
     */
    void onThemeChanged(std::function<void(Theme)> callback) {
        m_theme_changed_callbacks.push_back(std::move(callback));
    }

private:
    ThemeManager();
    ~ThemeManager() = default;

    /**
     * @brief 初始化预定义主题
     */
    void initializeThemes();

    /**
     * @brief 应用暗色主题
     */
    void applyDarkTheme();

    /**
     * @brief 应用亮色主题
     */
    void applyLightTheme();

    /**
     * @brief 应用经典主题
     */
    void applyClassicTheme();

    /**
     * @brief 触发主题变更回调
     */
    void notifyThemeChanged();

private:
    Theme m_current_theme = Theme::Dark;
    std::unordered_map<std::string, ImVec4> m_colors;
    std::vector<std::function<void(Theme)>> m_theme_changed_callbacks;
};

} // namespace DearTs::Core::UI
