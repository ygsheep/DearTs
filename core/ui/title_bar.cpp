/**
 * @file title_bar.cpp
 * @brief 自定义窗口标题栏实现
 */

#include "title_bar.h"
#include "logger.h"
#include <SDL3/SDL.h>
#include <algorithm>

namespace DearTs::Core::UI {

// ================ TitleBar Implementation ================

TitleBar::TitleBar() {
    LOG_INFO("TitleBar created");
}

bool TitleBar::render(const std::string& window_title, float window_width) {
    bool request_close = false;

    // 计算标题栏高度
    const float title_bar_height = get_title_bar_height();

    // 使用当前窗口的 DrawList（不是 ForegroundDrawList）
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_screen_pos = ImGui::GetWindowPos();

    // 计算标题栏区域
    ImVec2 title_bar_min = ImVec2(0, 0); // 窗口本地坐标
    ImVec2 title_bar_max = ImVec2(window_width, title_bar_height);

    // 使用稍微暗一点的背景色
    ImU32 title_bar_color = ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    draw_list->AddRectFilled(title_bar_min, title_bar_max, title_bar_color);

    // 顶部边框线
    draw_list->AddLine(title_bar_min,
                       ImVec2(title_bar_max.x, title_bar_min.y),
                       ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.3f, 1.0f)),
                       1.0f);

    // 保存原始光标位置
    ImVec2 original_cursor = ImGui::GetCursorPos();

    // 计算可用区域
    const float padding = 10.0f;
    const float button_size = 30.0f;

    // 1. 渲染自定义按钮（从左到右）
    // 手动计算 Y 坐标确保按钮背景垂直居中
    float button_y = (title_bar_height - button_size) / 2.0f;
    ImGui::SetCursorPos(ImVec2(padding, button_y));

    for (const auto& btn : m_custom_buttons) {
        ImVec2 button_size_vec(button_size, button_size);

        ImGui::PushStyleColor(ImGuiCol_Button, btn.color);
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(btn.color.x * 1.2f, btn.color.y * 1.2f, btn.color.z * 1.2f, btn.color.w));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(btn.color.x * 0.8f, btn.color.y * 0.8f, btn.color.z * 0.8f, btn.color.w));
        // 设置 FramePadding 为 0，确保按钮实际大小等于指定大小
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

        if (ImGui::Button(btn.icon, button_size_vec)) {
            if (btn.callback) {
                btn.callback();
            }
        }

        if (ImGui::IsItemHovered() && (btn.tooltip != nullptr)) {
            ImGui::SetTooltip("%s", btn.tooltip);
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0.0f, padding);
    }

    // 2. 渲染窗口标题（居中）
    const ImVec2 title_size = ImGui::CalcTextSize(window_title.c_str());
    const float title_x = (window_width - title_size.x) / 2.0f;
    const float title_y = (title_bar_height - title_size.y) / 2.0f;

    ImGui::SetCursorPos(ImVec2(title_x, title_y));
    ImGui::TextColored(ImVec4(1, 1, 1, 1), "%s", window_title.c_str());

    // 恢复光标位置
    ImGui::SetCursorPos(original_cursor);

    // 3. 处理拖拽（使用屏幕坐标）
    ImVec2 mouse_pos = ImGui::GetMousePos();
    handle_dragging(mouse_pos, title_bar_height, window_screen_pos, window_width);

    // 4. 渲染系统控制按钮（右侧）
    if (m_borderless) {
        ImGui::SetCursorPos(ImVec2(0, 0)); // 重置到窗口顶部
        request_close = render_system_buttons(window_width, title_bar_height);
    }

    return request_close;
}

bool TitleBar::render_button(const char* label, const ImVec2& size, const ImVec4& color) {
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(color.x * 1.2f, color.y * 1.2f, color.z * 1.2f, color.w));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(color.x * 0.8f, color.y * 0.8f, color.z * 0.8f, color.w));
    // 设置 FramePadding 为 0，确保按钮实际大小等于指定大小
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

    bool clicked = ImGui::Button(label, size);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    return clicked;
}

bool TitleBar::render_system_buttons(float window_width, float title_bar_height) {
    bool request_close = false;

    const float button_size = 30.0f;
    const float padding = 8.0f;

    // 手动计算 Y 坐标确保按钮背景垂直居中
    float button_y = (title_bar_height - button_size) / 2.0f;

    // 从右向左排列按钮
    float cursor_x = window_width - button_size - padding;

    // 关闭按钮（红色）
    ImGui::SetCursorPos(ImVec2(cursor_x, button_y));
    if (this->render_button(
            "✕", ImVec2(button_size, button_size), ImVec4(0.8f, 0.2f, 0.2f, 1.0f))) {
        request_close = true;
    }
    cursor_x -= button_size + padding;

    // 最大化/还原按钮
    ImGui::SetCursorPos(ImVec2(cursor_x, button_y));
    if (this->render_button(
            "□", ImVec2(button_size, button_size), ImVec4(0.5f, 0.5f, 0.5f, 1.0f))) {
        WindowControls::maximize_window();
    }
    cursor_x -= button_size + padding;

    // 最小化按钮
    ImGui::SetCursorPos(ImVec2(cursor_x, button_y));
    if (this->render_button(
            "-", ImVec2(button_size, button_size), ImVec4(0.5f, 0.5f, 0.5f, 1.0f))) {
        WindowControls::minimize_window();
    }

    return request_close;
}

void TitleBar::handle_dragging(const ImVec2& mouse_pos,
                               float title_bar_height,
                               const ImVec2& window_pos,
                               float window_width) {
    // 检查是否在标题栏区域（使用屏幕坐标）
    bool in_title_bar = mouse_pos.y >= window_pos.y &&
                        mouse_pos.y <= window_pos.y + title_bar_height &&
                        mouse_pos.x >= window_pos.x && mouse_pos.x <= window_pos.x + window_width;

    // 只有在标题栏区域时才开始拖拽
    if (in_title_bar && !m_is_dragging) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_is_dragging = true;

            // 获取 SDL 窗口的真实屏幕位置
            m_window_start_pos = WindowControls::get_window_position();

            // 使用 SDL 获取真实的鼠标屏幕位置
            float mouse_x, mouse_y;
            SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
            m_drag_start_pos = ImVec2(mouse_x, mouse_y);

            // 计算鼠标相对于 SDL 窗口左上角的偏移
            m_drag_offset = ImVec2(m_drag_start_pos.x - m_window_start_pos.x,
                                   m_drag_start_pos.y - m_window_start_pos.y);
        }
    }

    if (m_is_dragging) {
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_is_dragging = false;
        } else {
            // 使用 SDL 获取当前真实的鼠标屏幕位置
            float mouse_x, mouse_y;
            SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
            ImVec2 current_mouse_pos(mouse_x, mouse_y);

            // 只有在鼠标真正移动时才更新窗口位置，减少抖动
            float delta_x = current_mouse_pos.x - m_drag_start_pos.x;
            float delta_y = current_mouse_pos.y - m_drag_start_pos.y;

            if (std::abs(delta_x) > 0.5f || std::abs(delta_y) > 0.5f) {
                // 基于初始窗口位置和鼠标偏移计算新位置
                ImVec2 new_pos(m_window_start_pos.x + current_mouse_pos.x - m_drag_start_pos.x,
                               m_window_start_pos.y + current_mouse_pos.y - m_drag_start_pos.y);

                WindowControls::set_window_position(static_cast<int>(new_pos.x),
                                                    static_cast<int>(new_pos.y));

                // 更新起始位置，避免累积误差
                m_drag_start_pos = current_mouse_pos;
                m_window_start_pos = new_pos;
            }
        }
    }
}

float TitleBar::get_title_bar_height() const {
    float height = ImGui::GetFontSize() * 1.5f + 10.0f;

#ifdef __APPLE__
    height *= 0.7f; // macOS 标题栏稍短
#endif

    return height;
}

void TitleBar::add_button(const char* icon,
                          const char* tooltip,
                          ButtonCallback callback,
                          const ImVec4& color) {
    m_custom_buttons.push_back({icon, tooltip, callback, color});
}

void TitleBar::clear_buttons() {
    m_custom_buttons.clear();
}

// ================ Platform-specific Window Controls ================

namespace WindowControls {

static SDL_Window* g_current_window = nullptr;

void set_current_window(SDL_Window* window) {
    g_current_window = window;
}

void minimize_window() {
    if (g_current_window) {
        LOG_INFO("Minimize window requested");
        SDL_MinimizeWindow(g_current_window);
    }
}

void maximize_window() {
    // TODO: 实现最大化/还原功能
    LOG_INFO("Maximize/Restore window requested");
    // SDL3 需要手动处理窗口大小变化
}

void close_window() {
    LOG_INFO("Close window requested");
    SDL_Event event;
    event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&event);
}

void set_window_position(int x, int y) {
    if (g_current_window) {
        SDL_SetWindowPosition(g_current_window, x, y);
    }
}

ImVec2 get_window_position() {
    if (g_current_window) {
        int x, y;
        SDL_GetWindowPosition(g_current_window, &x, &y);
        return ImVec2(static_cast<float>(x), static_cast<float>(y));
    }
    return ImVec2(0, 0);
}

} // namespace WindowControls

} // namespace DearTs::Core::UI
