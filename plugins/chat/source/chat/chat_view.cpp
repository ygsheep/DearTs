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

    ImGui::Separator();

    // 消息区域（占据全部可用空间）
    draw_message_area();
}

void ChatView::draw_message_area() {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) return;

    // 获取可用区域大小
    const ImVec2 avail_size = ImGui::GetContentRegionAvail();

    // 创建子窗口用于消息滚动
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    if (ImGui::BeginChild("##messages", avail_size, false,
                          ImGuiWindowFlags_None)) {
        // 如果没有消息
        if (current_conv->is_empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);
            ImGui::TextWrapped("开始新的对话");
            ImGui::TextWrapped("在输入窗口中输入消息，然后按 Enter 发送");
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

                    // 流式输出消息
                    if (message.is_streaming) {
                        // 流式输出时直接显示完整内容（不需要打字机效果）
                        // 打字机效果已由流式输出本身提供
                        // 自动滚动到底部
                        ImGui::SetScrollHereY(1.0f);

                        // 绘制消息气泡
                        UI::MessageBubble::draw(message, &m_bubble_style);
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

void ChatView::scroll_to_bottom() {
    m_should_scroll_to_bottom = true;
}

} // namespace DearTs::Plugins::Chat
