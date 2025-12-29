#include "view.h"
#include "liblogger/logger.h"
#include <algorithm>

namespace DearTs {
namespace Core {
namespace UI {

// ================ View Base Implementation ================

View::View(UnlocalizedString unlocalized_name, const char* icon)
    : m_unlocalized_name(std::move(unlocalized_name))
    , m_icon(icon) {
}

bool View::should_draw() const {
    return m_window_open;
}

bool View::should_process() const {
    return true;
}

bool View::has_view_menu_entry() const {
    return true;
}

ImVec2 View::get_min_size() const {
    return ImVec2(400, 300);
}

ImVec2 View::get_max_size() const {
    return ImVec2(FLT_MAX, FLT_MAX);
}

ImGuiWindowFlags View::get_window_flags() const {
    return 0;
}

std::string View::get_name() const {
    return m_unlocalized_name.get();
}

bool& View::get_window_open_state() {
    return m_window_open;
}

const bool& View::get_window_open_state() const {
    return m_window_open;
}

bool View::did_window_just_open() {
    return m_window_just_opened;
}

bool View::did_window_just_close() {
    return m_window_just_closed;
}

void View::bring_to_front() {
    ImGui::SetNextWindowFocus();
}

void View::track_window_state() {
    m_window_just_opened = (m_window_open && !m_prev_window_open);
    m_window_just_closed = (!m_window_open && m_prev_window_open);

    if (m_window_just_opened) {
        on_open();
    } else if (m_window_just_closed) {
        on_close();
    }

    m_prev_window_open = m_window_open;
}

void View::set_focused(bool focused) {
    m_focused = focused;
}

std::string View::to_window_name(const UnlocalizedString& unlocalized_name) {
    return unlocalized_name.get();
}

// ================ ViewWindow Implementation ================

ViewWindow::ViewWindow(UnlocalizedString unlocalized_name, const char* icon)
    : View(std::move(unlocalized_name), icon) {
}

void ViewWindow::draw(ImGuiWindowFlags extra_flags) {
    if (!m_window_open) {
        return;
    }

    ImGui::SetNextWindowSizeConstraints(get_min_size(), get_max_size());

    // 如果设置了初始位置且窗口刚刚打开，则设置窗口位置
    if (m_initial_position_set && m_window_just_opened) {
        ImGui::SetNextWindowPos(m_initial_position, ImGuiCond_FirstUseEver);
        m_initial_position_set = false;  // 只设置一次
    }

    auto window_name = to_window_name(m_unlocalized_name);
    auto flags = get_window_flags() | extra_flags;

    if (ImGui::Begin(window_name.c_str(), &m_window_open, flags)) {
        m_focused = ImGui::IsWindowFocused();

        // 绘制帮助文本（如果有）
        draw_help_text();

        // 绘制内容
        ImGui::BeginChild("##content");
        draw_content();
        ImGui::EndChild();
    }

    ImGui::End();

    track_window_state();
}

// ================ ViewSpecial Implementation ================

ViewSpecial::ViewSpecial(UnlocalizedString unlocalized_name)
    : View(std::move(unlocalized_name), "") {
}

void ViewSpecial::draw(ImGuiWindowFlags extra_flags) {
    draw_content();
}

// ================ ViewFloating Implementation ================

ViewFloating::ViewFloating(UnlocalizedString unlocalized_name, const char* icon)
    : ViewWindow(std::move(unlocalized_name), icon) {
}

void ViewFloating::draw(ImGuiWindowFlags extra_flags) {
    if (!m_window_open) {
        return;
    }

    ImGui::SetNextWindowSizeConstraints(get_min_size(), get_max_size());

    auto window_name = to_window_name(m_unlocalized_name);
    // 注意: ImGui 1.92.x 已移除 ImGuiWindowFlags_NoDocking，停靠功能已默认启用或通过其他方式控制
    auto flags = get_window_flags() | extra_flags;

    if (ImGui::Begin(window_name.c_str(), &m_window_open, flags)) {
        m_focused = ImGui::IsWindowFocused();
        draw_content();
    }

    ImGui::End();

    track_window_state();
}

// ================ ViewModal Implementation ================

ViewModal::ViewModal(UnlocalizedString unlocalized_name, const char* icon)
    : View(std::move(unlocalized_name), icon) {
}

void ViewModal::draw(ImGuiWindowFlags extra_flags) {
    if (!m_window_open) {
        return;
    }

    ImGui::SetNextWindowSizeConstraints(get_min_size(), get_max_size());

    auto window_name = to_window_name(m_unlocalized_name);
    // 注意: ImGui 1.92.x 已移除 ImGuiWindowFlags_NoDocking
    // 对于没有关闭按钮的模态窗口，使用 nullptr 作为 p_open 参数
    auto flags = get_window_flags() | extra_flags | ImGuiWindowFlags_Modal;

    // 打开模态窗口
    // 如果没有关闭按钮，传递 nullptr 给 p_open 参数
    bool* p_open = has_close_button() ? &m_window_open : nullptr;
    if (ImGui::BeginPopupModal(window_name.c_str(), p_open, flags)) {
        m_focused = ImGui::IsWindowFocused();
        draw_content();

        if (!m_window_open) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    } else if (m_window_open) {
        // 窗口刚打开
        ImGui::OpenPopup(window_name.c_str());
    }

    track_window_state();
}

} // namespace UI

// ================ Views Registry Implementation ================

namespace ContentRegistry {
namespace Views {

namespace {
    std::map<UnlocalizedString, std::unique_ptr<UI::View>> g_views;
    UI::View* g_focused_view = nullptr;
}

namespace impl {

void add(std::unique_ptr<UI::View>&& view) {
    auto name = view->get_unlocalized_name();

    if (g_views.find(name) != g_views.end()) {
        LOG_WARN("View '{}' already exists, overwriting", name.get());
    }

    g_views[name] = std::move(view);
    LOG_INFO("Added view: {}", name.get());
}

} // namespace impl

UI::View* get_by_name(const UnlocalizedString& unlocalized_name) {
    auto it = g_views.find(unlocalized_name);
    if (it == g_views.end()) {
        return nullptr;
    }
    return it->second.get();
}

UI::View* get_focused() {
    return g_focused_view;
}

const std::map<UnlocalizedString, std::unique_ptr<UI::View>>& get_all() {
    return g_views;
}

} // namespace Views
} // namespace ContentRegistry

} // namespace Core
} // namespace DearTs
