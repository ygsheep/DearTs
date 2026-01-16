/**
 * @file chat_view.cpp
 * @brief 聊天视图实现
 */

#include "chat/views/chat_view.hpp"
#include "chat/events/chat_events.hpp"
#include "liblogger/logger.h"
#include <imgui.h>
#include <format>
#include <cmath>

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

    // 禁用整个视图的滚动条（只有消息区域子窗口会滚动）
    const float total_height = ImGui::GetContentRegionAvail().y;
    const float input_height = 120.0f;  // 输入框区域预留高度
    const float message_height = total_height - input_height;  // 消息区域高度

    // 消息区域（占据剩余空间，可滚动）
    draw_message_area(message_height);

    ImGui::Separator();

    // AI 建议区域（如果有建议）
    if (m_show_suggestions && !m_suggestions.empty()) {
        draw_suggestions();
        ImGui::Separator();
    }

    // 输入区域（固定高度，不滚动）
    draw_input_area();
}

void ChatView::draw_message_area(float height) {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) return;

    // 创建子窗口用于消息滚动（使用传入的高度）
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    if (ImGui::BeginChild("##messages", ImVec2(0, height), false,
                          ImGuiWindowFlags_None)) {
        // 如果没有消息
        if (current_conv->is_empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);
            ImGui::TextWrapped("开始新的对话");
            ImGui::TextWrapped("在下方输入消息，然后按 Enter 发送");
            ImGui::PopStyleColor();
        } else {
            // 检查是否有流式输出的消息
            bool has_streaming = false;
            for (const auto& msg : current_conv->messages) {
                if (msg.is_streaming) {
                    has_streaming = true;
                    break;
                }
            }

            // 如果有流式输出，不使用虚拟滚动（因为内容每帧变化）
            // 否则使用虚拟滚动提高性能
            if (has_streaming) {
                // 流式输出模式：渲染所有消息
                for (size_t i = 0; i < current_conv->messages.size(); i++) {
                    auto& message = current_conv->messages[i];

                    // 添加一些垂直间距
                    if (i > 0) {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                    }

                    // 流式输出消息的打字机效果
                    if (message.is_streaming) {
                        // 逐步增加显示字符（打字机效果）
                        if (message.displayed_chars < message.content.length()) {
                            // 每帧增加 2-3 个字符（可配置速度）
                            const size_t chars_per_frame = 3;
                            message.displayed_chars = std::min(
                                message.displayed_chars + chars_per_frame,
                                message.content.length()
                            );

                            // 流式完成检查
                            if (message.displayed_chars >= message.content.length()) {
                                message.is_streaming = false;
                            }

                            // 自动滚动到底部
                            ImGui::SetScrollHereY(1.0f);
                        }

                        // 创建临时消息对象，仅显示已渲染的部分
                        Message display_message = message;
                        display_message.content = message.content.substr(0, message.displayed_chars);

                        // 绘制消息气泡
                        UI::MessageBubble::draw(display_message, &m_bubble_style);
                    } else {
                        // 绘制完整的消息气泡
                        UI::MessageBubble::draw(message, &m_bubble_style);
                    }
                }
            } else {
                // 普通模式：使用虚拟滚动提高性能
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
        const std::string label = std::format("##suggestion_{}", suggestion.id);

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
        ImVec2 item_min = ImGui::GetItemRectMin();
        const ImVec2 text_pos = ImVec2(item_min.x + 10, item_min.y + 5);
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

    const float button_size = 44.0f;
    const float send_button_size = 48.0f;
    const float button_spacing = 8.0f;
    const float button_margin = 12.0f;  // 按钮距离边框的距离
    const float total_button_width = button_size + button_spacing + send_button_size + button_margin;

    // 获取窗口绘制列表
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 获取可用区域大小
    ImVec2 avail_size = ImGui::GetContentRegionAvail();

    // 输入框样式 - 透明背景，和主背景一样
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16, 16));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));  // 透明背景
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));

    // 输入框填满整个可用区域
    const bool input_submitted = ImGui::InputTextMultiline(
        "##chat_input",
        m_input_buffer,
        sizeof(m_input_buffer),
        avail_size,
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CtrlEnterForNewLine
    );

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    // 获取输入框位置信息（屏幕坐标）
    const bool is_input_focused = ImGui::IsItemFocused();
    const ImVec2 input_min = ImGui::GetItemRectMin();
    const ImVec2 input_max = ImGui::GetItemRectMax();

    // 绘制输入框边框（如果需要的话）
    if (is_input_focused) {
        const ImU32 focus_color = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.55f, 0.95f, 0.6f));
        draw_list->AddRect(input_min, input_max, focus_color, 12.0f, 0, 2.5f);
    } else {
        // 默认边框
        const ImU32 border_color = ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.15f, 0.18f, 0.5f));
        draw_list->AddRect(input_min, input_max, border_color, 12.0f, 0, 1.0f);

        // 如果输入框未聚焦，自动请求焦点（发送消息后保持聚焦）
        if (m_input_focused) {
            ImGui::SetKeyboardFocusHere(-1);  // -1 表示聚焦到上一个 item（输入框）
            m_input_focused = false;
        }
    }

    // 使用屏幕坐标定位按钮（覆盖在输入框上层）
    const ImVec2 ai_button_screen_pos(
        input_max.x - total_button_width,
        input_max.y - button_margin - button_size
    );
    const ImVec2 send_button_screen_pos(
        input_max.x - button_margin - send_button_size,
        input_max.y - button_margin - send_button_size
    );

    // AI 分析按钮
    ImGui::SetCursorScreenPos(ai_button_screen_pos);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, button_size / 2);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.45f, 0.75f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.5f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.55f, 0.9f, 1.0f));

    const bool ai_analyze_clicked = ImGui::Button("##ai_analyze", ImVec2(button_size, button_size));

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    // AI 按钮图标
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("AI 分析");
    }
    const ImVec2 ai_btn_min = ImGui::GetItemRectMin();
    draw_list->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize(),
        ImVec2(ai_btn_min.x + (button_size - ImGui::CalcTextSize(ICON_AUTO_FIX_HIGH).x) / 2,
              ai_btn_min.y + (button_size - ImGui::GetFontSize()) / 2),
        IM_COL32(200, 220, 255, 255),
        ICON_AUTO_FIX_HIGH
    );

    // 发送按钮
    ImGui::SetCursorScreenPos(send_button_screen_pos);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, send_button_size / 2);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.8f, 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.85f, 0.7f, 1.0f));

    const bool send_clicked = ImGui::Button("##send", ImVec2(send_button_size, send_button_size));

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    // 发送按钮图标
    const bool send_hovered = ImGui::IsItemHovered();
    const ImVec2 send_btn_min = ImGui::GetItemRectMin();
    const ImU32 send_icon_color = send_hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 255, 230, 255);
    draw_list->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize() * 1.2f,
        ImVec2(send_btn_min.x + (send_button_size - ImGui::CalcTextSize(ICON_SEND).x * 1.2f) / 2,
              send_btn_min.y + (send_button_size - ImGui::GetFontSize() * 1.2f) / 2 - 1),
        send_icon_color,
        ICON_SEND
    );

    // 添加提示
    if (send_hovered) {
        ImGui::SetTooltip("发送消息 (Enter)\nCtrl+Enter 换行");
    }

    // 处理发送
    const double current_time = ImGui::GetTime();
    const bool cooldown_ok = (current_time - m_last_send_time) > 0.5;

    // Enter 键触发
    if (input_submitted && cooldown_ok) {
        send_message();
    }

    // AI 分析按钮
    if (ai_analyze_clicked && cooldown_ok) {
        request_ai_analysis();
    }

    // 发送按钮
    if (send_clicked && cooldown_ok) {
        send_message();
    }

    // AI 分析动画
    if (m_analyzing) {
        const float time = static_cast<float>(ImGui::GetTime());
        const float spin_offset = std::sin(time * 5.0f) * 3.0f;
        draw_list->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize() * 0.9f,
            ImVec2(ai_btn_min.x + button_size / 2 - ImGui::CalcTextSize(ICON_SYNC).x / 2 + spin_offset,
                  ai_btn_min.y + button_size + 4),
            IM_COL32(100, 150, 255, 255),
            ICON_SYNC
        );
    }
}

void ChatView::draw_suggestion_chip(const AISuggestion& suggestion) {
    // 已在 draw_suggestions 中实现
}

void ChatView::send_message() {
    // 设置发送标志和时间戳（防止重复发送）
    m_sending = true;
    m_last_send_time = ImGui::GetTime();

    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) {
        LOG_WARN("send_message: No current conversation");
        m_sending = false;
        return;
    }

    // 获取输入框内容并移除末尾的换行符
    std::string content(m_input_buffer);
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r')) {
        content.pop_back();
    }

    LOG_INFO("send_message called: buffer='{}', cleaned='{}', m_sending={}, time={}",
             std::string(m_input_buffer), content, m_sending, m_last_send_time);

    if (content.empty()) {
        LOG_INFO("send_message: content empty after cleaning, returning");
        m_input_buffer[0] = '\0';  // 清空输入框
        m_sending = false;
        return;
    }

    // 立即清空输入框（防止重复发送）
    m_input_buffer[0] = '\0';

    // 创建消息
    Message message(content, MessageRole::User);
    message.status = MessageStatus::Sending;

    // 添加到会话
    current_conv->add_message(message);
    current_conv->touch();

    // 滚动到底部
    m_should_scroll_to_bottom = true;

    // 发布消息发送事件
    DearTs::Core::Event::EventBus::instance().publish(Events::MessageSentEvent{
        .conversation_id = current_conv->id,
        .message = message
    });

    LOG_INFO("Sent message in conversation {}: {}", current_conv->id, content);

    // 自动触发 AI 分析请求（发送消息后立即获取 AI 响应）
    DearTs::Core::Event::EventBus::instance().publish(Events::AIAnalysisRequestEvent{
        .conversation_id = current_conv->id,
        .context = {},  // 留空，LLM 提供商会从会话中获取上下文
        .current_message = message,
        .suggestion_count = 0  // 不需要建议，只需要响应
    });

    LOG_INFO("Requested AI response for conversation {}", current_conv->id);

    // 清空之前的建议
    m_suggestions.clear();

    // 保持输入框聚焦
    m_input_focused = true;
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
        DearTs::Core::Event::EventBus::instance().publish(Events::SuggestionChipClickedEvent{
            .conversation_id = current_conv->id,
            .suggestion = suggestion
        });
    }
}

void ChatView::request_ai_analysis() {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) return;

    // 发布 AI 分析请求事件
    DearTs::Core::Event::EventBus::instance().publish(Events::AIAnalysisRequestEvent{
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
