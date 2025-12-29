/**
 * @file sidebar_view.hpp
 * @brief 侧边栏视图
 * @details 提供视图导航和状态管理功能
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/ui/view.h"
#include <string>

namespace DearTs::Plugins::Navigation {

/**
 * @brief 视图分类
 */
enum class ViewCategory {
    All,
    Core,
    Tools,
    Plugins
};

/**
 * @brief 侧边栏视图类
 */
class SidebarView : public Core::UI::ViewWindow {
public:
    SidebarView();
    ~SidebarView() override = default;

    /**
     * @brief 绘制视图内容
     */
    void draw_content() override;

private:
    /**
     * @brief 绘制工具栏
     */
    void draw_toolbar();

    /**
     * @brief 绘制搜索框
     */
    void draw_search_box();

    /**
     * @brief 绘制筛选器
     */
    void draw_filter();

    /**
     * @brief 绘制视图列表
     */
    void draw_view_list(const std::string& category);

    /**
     * @brief 绘制视图项
     */
    void draw_view_item(Core::UI::View* view);

    /**
     * @brief 应用筛选
     */
    void apply_filter();

    /**
     * @brief 获取分类显示名称
     */
    const char* get_category_name(ViewCategory category) const;

    /**
     * @brief 绘制设置按钮
     */
    void draw_settings_button();

private:
    // UI 状态
    char m_search_buffer[256] = "";
    ViewCategory m_current_category = ViewCategory::All;

    // 统计
    int m_visible_count = 0;
    int m_total_count = 0;
};

} // namespace DearTs::Plugins::Navigation
