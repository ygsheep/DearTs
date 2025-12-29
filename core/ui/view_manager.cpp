/**
 * @file view_manager.cpp
 * @brief 视图管理器实现
 */

#include "view_manager.h"
#include "layout_manager.h"
#include "logger.h"

namespace DearTs::Core::UI {

void ViewManager::addView(std::unique_ptr<View> view) {
    if (!view) {
        LOG_ERROR("尝试添加空视图");
        return;
    }

    std::string name = view->get_name();
    m_views[name] = std::move(view);

    LOG_INFO("添加视图: {}", name);
}

void ViewManager::removeView(const std::string& name) {
    auto it = m_views.find(name);
    if (it != m_views.end()) {
        m_views.erase(it);
        LOG_INFO("移除视图: {}", name);
    }
}

View* ViewManager::getViewByName(const std::string& name) const {
    auto it = m_views.find(name);
    if (it != m_views.end()) {
        return it->second.get();
    }
    return nullptr;
}

View* ViewManager::getFocusedView() const {
    for (const auto& [name, view] : m_views) {
        if (view->is_focused()) {
            return view.get();
        }
    }
    return nullptr;
}

void ViewManager::renderAllViews() {
    for (auto& [name, view] : m_views) {
        // 跟踪视图状态
        view->track_window_state();

        // 检查是否需要处理
        if (!view->should_process()) {
            continue;
        }

        // 绘制视图
        ImGuiWindowFlags flags = view->get_window_flags();

        // 如果布局锁定，添加额外的标志
        if (m_locked) {
            flags |= ImGuiWindowFlags_NoMove;
            flags |= ImGuiWindowFlags_NoResize;
        }

        view->draw(flags);

        // 处理视图停靠（仅在第一次打开时）
        if (view->did_window_just_open() && m_main_dock_space_id != 0) {
            handleViewDocking(view.get());
        }
    }
}

void ViewManager::createMainDockSpace(const ImVec2& viewport_size) {
    // 注意：主 DockSpace 已在 render_demo_window() 中创建
    // 这个函数保留用于兼容性，但不再需要实际创建 DockSpace
    m_dock_built = true;
    (void)viewport_size;
}

void ViewManager::closeAllViews() {
    for (auto& [name, view] : m_views) {
        view->get_window_open_state() = false;
    }
    LOG_INFO("关闭所有视图");
}

void ViewManager::handleViewDocking(View* view) {
    // 简化版本：ImGui 的停靠系统会自动处理窗口停靠
    // 不需要手动调用 DockBuilder API
    // 窗口会根据用户拖动自动停靠到合适的位置
    (void)view; // 避免未使用参数警告
}

} // namespace DearTs::Core::UI
