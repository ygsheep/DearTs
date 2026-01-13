/**
 * @file chat_view.cpp
 * @brief 聊天视图实现
 */

#include "chat/views/chat_view.hpp"
#include "chat/events/chat_events.hpp"
#include "liblogger/logger.h"
#include <imgui.h>
#include <fmt/format.h>

namespace DearTs::Plugins::Chat {

void ChatView::draw_content() {
    auto current_conv = m_conversation_manager->get_current_conversation();

    // 如果没有选中的会话
    if (!current_conv) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 100);
        ImGui::TextWrapped("请选择或创建一个会话开始聊天");
        ImGui::PopStyleColor();
        return;
    }

    // 消息区域（占据大部分空间）
    draw_message_area();

    ImGui::Separator();

    // AI 建议区域（如果有建议）
    if (m_show_suggestions && !m_suggestions.empty()) {
        draw_suggestions();
        ImGui::Separator();
    }

    // 输入区域
    draw_input_area();
}

void ChatView::draw_message_area() {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) return;

    // 设置可用空间（减去输入区域和边距）
    const float input_height = 100;
    const float avail_height = ImGui::GetContentRegionAvail().y - input_height - 20;

    // 创建子窗口用于消息滚动
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    if (ImGui::BeginChild("##messages", ImVec2(0, avail_height), false,
                          ImGuiWindowFlags_None)) {
        // 如果没有消息
        if (current_conv->is_empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);
            ImGui::TextWrapped("开始新的对话");
            ImGui::TextWrapped("在下方输入消息，然后按 Enter 发送");
            ImGui::PopStyleColor();
        } else {
            // 使用虚拟滚动绘制消息
            ImGuiListClipper clipper;
            clipper.Begin(current_conv->get_message_count());

            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    const auto& message = current_conv->messages[i];

                    // 添加一些垂直间距
                    if (i > 0) {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                    }

                    // 绘制消息气泡
                    UI::MessageBubble::draw(message, &m_bubble_style);
                }
            }

            clipper.End();
        }

        // 自动滚动到底部
        if (m_should_scroll_to_bottom && m_auto_scroll) {
            ImGui::SetScrollHereY(1.0f);
            m_should_scroll_to_bottom = false;
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // 如果用户向上滚动，禁用自动滚动
    if (ImGui::GetScrollY() < ImGui::GetScrollMaxY() - 50) {
        m_auto_scroll = false;
    } else {
        m_auto_scroll = true;
    }
}

void ChatView::draw_suggestions() {
    ImGui::Text("💡 AI 建议回复:");

    // 水平滚动建议芯片
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

    for (const auto& suggestion : m_suggestions) {
        ImGui::SameLine();

        // 建议芯片按钮
        const std::string label = fmt::format("##suggestion_{}", suggestion.id);

        // 计算按钮大小
        const ImVec2 text_size = ImGui::CalcTextSize(suggestion.content.c_str());
        const ImVec2 button_size = ImVec2(text_size.x + 20, text_size.y + 10);

        // 绘制芯片
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.23f, 1.0f));

        if (ImGui::Button(label.c_str(), button_size)) {
            on_suggestion_clicked(suggestion);
        }

        ImGui::PopStyleColor(3);

        // 绘制文本（居中）
        const ImVec2 text_pos = ImGui::GetItemRectMin() + ImVec2(10, 5);
        ImGui::GetWindowDrawList()->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize(),
            text_pos,
            IM_COL32(200, 200, 200, 255),
            suggestion.content.c_str()
        );
    }

    ImGui::PopStyleVar();
}

void ChatView::draw_input_area() {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) return;

    // 输入框
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.10f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

    const bool input_enter = ImGui::InputTextMultiline(
        "##chat_input",
        m_input_buffer,
        sizeof(m_input_buffer),
        ImVec2(-1, ImGui::GetFontSize() * 3),
        ImGuiInputTextFlags_EnterReturnsTrue
    );

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    // 输入框获得焦点
    if (m_input_focused && !ImGui::IsItemFocused()) {
        ImGui::SetKeyboardFocusHere(-1);
        m_input_focused = false;
    }

    // 按钮栏
    ImGui::Spacing();

    const float button_width = 100;
    const float avail_width = ImGui::GetContentRegionAvail().x;

    // AI 分析按钮
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.13f, 0.5f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.55f, 0.85f, 1.0f));

    if (ImGui::Button(fmt::format("{} AI 分析", ICON_AUTO_FIX_HIGH).c_str(), ImVec2(button_width, 0))) {
        request_ai_analysis();
    }

    ImGui::PopStyleColor(2);

    ImGui::SameLine();

    // 发送按钮
    ImGui::SetCursorPosX(avail_width - button_width);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.13f, 0.75f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.8f, 0.55f, 1.0f));

    const bool send_clicked = ImGui::Button(fmt::format("{} 发送", ICON_SEND).c_str(), ImVec2(button_width, 0));

    ImGui::PopStyleColor(2);

    // 处理发送
    if ((input_enter && ImGui::IsItemFocused() && ImGui::GetIO().KeyShift) || send_clicked) {
        send_message();
    }

    // 如果正在分析，显示进度
    if (m_analyzing) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "%s 正在分析...", ICON_SYNC);
    }
}

void ChatView::draw_suggestion_chip(const AISuggestion& suggestion) {
    // 已在 draw_suggestions 中实现
}

void ChatView::send_message() {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) return;

    const std::string content(m_input_buffer);
    if (content.empty()) return;

    // 创建消息
    Message message(content, MessageRole::User);
    message.status = MessageStatus::Sending;

    // 添加到会话
    current_conv->add_message(message);
    current_conv->touch();

    // 清空输入框
    m_input_buffer[0] = '\0';
    m_input_focused = true;

    // 滚动到底部
    m_should_scroll_to_bottom = true;

    // 发布消息发送事件
    EventBus::instance().publish(Events::MessageSentEvent{
        .conversation_id = current_conv->id,
        .message = message
    });

    LOG_INFO("Sent message in conversation {}: {}", current_conv->id, content);

    // 清空之前的建议
    m_suggestions.clear();
}

void ChatView::on_suggestion_clicked(const AISuggestion& suggestion) {
    // 将建议填充到输入框
    strncpy(m_input_buffer, suggestion.content.c_str(), sizeof(m_input_buffer) - 1);
    m_input_buffer[sizeof(m_input_buffer) - 1] = '\0';
    m_input_focused = true;

    // 隐藏建议
    m_show_suggestions = false;

    // 发布事件
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (current_conv) {
        EventBus::instance().publish(Events::SuggestionChipClickedEvent{
            .conversation_id = current_conv->id,
            .suggestion = suggestion
        });
    }
}

void ChatView::request_ai_analysis() {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) return;

    // 发布 AI 分析请求事件
    EventBus::instance().publish(Events::AIAnalysisRequestEvent{
        .conversation_id = current_conv->id,
        .context = {},
        .current_message = {},
        .suggestion_count = 3
    });

    m_analyzing = true;

    LOG_INFO("Requested AI analysis for conversation {}", current_conv->id);
}

void ChatView::scroll_to_bottom() {
    m_should_scroll_to_bottom = true;
}

} // namespace DearTs::Plugins::Chat
