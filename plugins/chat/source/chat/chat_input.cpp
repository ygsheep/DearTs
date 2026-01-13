/**
 * @file chat_input.cpp
 * @brief 聊天输入框组件实现
 */

#include "chat/ui/chat_input.hpp"
#include <fmt/format.h>
#include <cstring>

namespace DearTs::Plugins::Chat::UI {

ChatInput::ChatInput() {
    m_buffer[0] = '\0';
}

bool ChatInput::draw(const char* placeholder, const ChatInputStyle* style_ptr) {
    ChatInputStyle default_style;
    const ChatInputStyle& style = style_ptr ? *style_ptr : default_style;

    bool should_send = false;

    // 设置焦点
    if (m_should_focus) {
        ImGui::SetKeyboardFocusHere(-1);
        m_should_focus = false;
    }

    // 绘制输入框
    ImGui::PushStyleColor(ImGuiCol_FrameBg, style.bg_color);
    ImGui::PushStyleColor(ImGuiCol_Border, style.border_color);
    ImGui::PushStyleColor(ImGuiCol_Text, style.text_color);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.corner_radius);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style.border_thickness);

    const float input_height = ImGui::GetFontSize() * 3;
    const bool input_enter = ImGui::InputTextMultiline(
        "##chat_input",
        m_buffer,
        sizeof(m_buffer),
        ImVec2(-1, input_height),
        ImGuiInputTextFlags_EnterReturnsTrue
    );

    // 更新焦点状态
    m_has_focus = ImGui::IsItemFocused();

    // 检测焦点变化
    static bool was_focused = false;
    if (m_has_focus && !was_focused) {
        // 刚获得焦点
    }
    was_focused = m_has_focus;

    // 检测文本变化
    if (ImGui::IsItemEdited()) {
        m_text = m_buffer;
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    // 处理键盘输入
    if (input_enter && m_has_focus) {
        // 检测 Shift+Enter（换行）或 Enter（发送）
        if (style.enter_sends && !ImGui::GetIO().KeyShift) {
            should_send = true;
        }
    }

    // 按钮栏
    ImGui::Spacing();
    ImGui::Spacing();

    const float button_width = 100;
    const float avail_width = ImGui::GetContentRegionAvail().x;

    // AI 分析按钮
    if (style.show_ai_button) {
        const bool ai_clicked = draw_ai_button();
        if (ai_clicked && m_ai_callback) {
            m_ai_callback();
        }
    }

    // 发送按钮（右侧）
    if (style.show_send_button) {
        ImGui::SameLine(avail_width - button_width);
        const bool send_clicked = draw_send_button();

        if (send_clicked && m_send_callback) {
            m_send_callback(m_text);
            should_send = true;
        }
    }

    return should_send;
}

void ChatInput::update_buffer() {
    strncpy(m_buffer, m_text.c_str(), sizeof(m_buffer) - 1);
    m_buffer[sizeof(m_buffer) - 1] = '\0';
}

bool ChatInput::handle_keyboard() {
    // 在 draw 函数中已处理
    return false;
}

bool ChatInput::draw_send_button() {
    const bool is_empty = m_text.empty();

    // 根据是否有内容设置按钮样式
    ImVec4 btn_color, btn_hovered, btn_active;
    if (is_empty) {
        btn_color = ImVec4(0.2f, 0.2f, 0.2f, 0.5f);
        btn_hovered = ImVec4(0.25f, 0.25f, 0.25f, 0.5f);
        btn_active = ImVec4(0.3f, 0.3f, 0.3f, 0.5f);
    } else {
        btn_color = ImVec4(0.13f, 0.75f, 0.5f, 1.0f);
        btn_hovered = ImVec4(0.15f, 0.8f, 0.55f, 1.0f);
        btn_active = ImVec4(0.1f, 0.7f, 0.45f, 1.0f);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, btn_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, btn_active);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    const bool clicked = ImGui::Button(fmt::format("{} 发送", ICON_SEND).c_str(), ImVec2(100, 0));

    ImGui::PopStyleColor(4);

    // 提示：Shift+Enter 换行
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enter 发送，Shift+Enter 换行");
    }

    return clicked && !is_empty;
}

bool ChatInput::draw_ai_button() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.13f, 0.5f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.55f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.45f, 0.75f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    const bool clicked = ImGui::Button(fmt::format("{} AI 分析", ICON_AUTO_FIX_HIGH).c_str(), ImVec2(100, 0));

    ImGui::PopStyleColor(4);

    return clicked;
}

} // namespace DearTs::Plugins::Chat::UI
