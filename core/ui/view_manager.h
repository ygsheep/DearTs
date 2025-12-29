/**
 * @file view_manager.h
 * @brief 视图管理器
 * @details 参考 ImHex 实现视图的注册、管理和停靠功能
 * @author DearTs Team
 * @date 2024
 * @version 1.0.0
 */

#pragma once

#include "view.h"
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace DearTs::Core::UI {

/**
 * @brief 视图管理器
 *
 * 管理所有视图的注册、绘制和停靠功能
 * 参考 ImHex 的 ContentRegistry::Views 系统
 */
class ViewManager {
public:
    /**
     * @brief 获取单例实例
     */
    static ViewManager& instance() {
        static ViewManager inst;
        return inst;
    }

    /**
     * @brief 添加视图
     * @param view 视图对象
     */
    void addView(std::unique_ptr<View> view);

    /**
     * @brief 移除视图
     * @param name 视图名称
     */
    void removeView(const std::string& name);

    /**
     * @brief 通过名称获取视图
     */
    [[nodiscard]] View* getViewByName(const std::string& name) const;

    /**
     * @brief 获取所有视图
     */
    [[nodiscard]] const std::map<std::string, std::unique_ptr<View>>& getAllViews() const {
        return m_views;
    }

    /**
     * @brief 获取当前聚焦的视图
     */
    [[nodiscard]] View* getFocusedView() const;

    /**
     * @brief 绘制所有视图
     */
    void renderAllViews();

    /**
     * @brief 创建主停靠空间
     * @param viewport_size 主视口大小
     */
    void createMainDockSpace(const ImVec2& viewport_size);

    /**
     * @brief 关闭所有视图
     */
    void closeAllViews();

    /**
     * @brief 设置布局锁定状态
     * @param locked 是否锁定
     */
    void setLayoutLocked(bool locked) { m_locked = locked; }

    /**
     * @brief 检查布局是否锁定
     */
    [[nodiscard]] bool isLayoutLocked() const { return m_locked; }

private:
    ViewManager() = default;
    ~ViewManager() = default;

    // 禁止拷贝
    ViewManager(const ViewManager&) = delete;
    ViewManager& operator=(const ViewManager&) = delete;

    /**
     * @brief 处理视图停靠
     * @param view 视图指针
     */
    void handleViewDocking(View* view);

private:
    std::map<std::string, std::unique_ptr<View>> m_views;
    ImGuiID m_main_dock_space_id = 0;
    bool m_locked = false;
    bool m_dock_built = false;
};

} // namespace DearTs::Core::UI
