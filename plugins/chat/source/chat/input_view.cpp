/**
 * @file input_view.cpp
 * @brief 输入视图实现
 */

#include "chat/views/input_view.hpp"
#include "chat/views/chat_view.hpp"
#include "chat/events/chat_events.hpp"
#include "chat/ui/markdown_renderer.hpp"
#include "core/ui/theme_manager.h"
#include "liblogger/logger.h"
#include <imgui.h>
#include <format>
#include <cmath>

namespace DearTs::Plugins::Chat {

// 输入模式枚举
enum class InputMode {
    Edit,    // 编辑模式
    Preview  // 预览模式
};

void InputView::draw_content() {
    auto current_conv = m_conversation_manager->get_current_conversation();

    // 如果没有选中的会话
    if (!current_conv) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::TextWrapped("请选择或创建一个会话开始聊天");
        ImGui::PopStyleColor();
        return;
    }

    // 静态变量用于输入模式切换
    static InputMode input_mode = InputMode::Edit;

    // Tab 切换按钮
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16, 8));

    if (ImGui::BeginTabBar("##input_mode_tab", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("编辑")) {
            input_mode = InputMode::Edit;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("预览")) {
            input_mode = InputMode::Preview;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::PopStyleVar(2);

    ImGui::Spacing();

    // 根据模式显示不同内容
    if (input_mode == InputMode::Edit) {
        draw_input_area();
    } else {
        draw_preview_area();
    }
}

void InputView::draw_input_area() {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) return;

    // 计算输入框高度（占据剩余空间减去按钮区域）
    const float button_height = 60.0f;
    ImVec2 avail_size = ImGui::GetContentRegionAvail();
    ImVec2 input_size(avail_size.x, avail_size.y - button_height - 10.0f);

    // 多行输入框
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16, 16));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));  // 透明背景
    ImGui::PushStyleColor(ImGuiCol_Border, DearTs::Core::UI::ThemeManager::instance().getColor("chat.input_border"));  // 主题边框色

    ImGui::InputTextMultiline(
        "##chat_input",
        m_input_buffer,
        sizeof(m_input_buffer),
        input_size,
        ImGuiInputTextFlags_AllowTabInput
    );

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    ImGui::Spacing();

    // 按钮区域（在输入框下方）
    draw_button_area();
}

void InputView::draw_preview_area() {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) return;

    // 计算预览区域高度
    const float button_height = 60.0f;
    ImVec2 avail_size = ImGui::GetContentRegionAvail();
    ImVec2 preview_size(avail_size.x, avail_size.y - button_height - 10.0f);

    // 创建预览子窗口（关闭内置边框以保持圆角）
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.08f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, DearTs::Core::UI::ThemeManager::instance().getColor("chat.input_border"));

    if (ImGui::BeginChild("##preview", preview_size, false,
                          ImGuiWindowFlags_None)) {
        // 获取输入内容
        std::string input_text(m_input_buffer);

        if (input_text.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);
            ImGui::TextWrapped("在编辑模式下输入内容，这里将显示 Markdown 预览...");
            ImGui::PopStyleColor();
        } else {
            // 使用 MarkdownRenderer 渲染预览
            UI::MarkdownRenderer::render(input_text);
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    ImGui::Spacing();

    // 按钮区域（在预览下方）
    draw_button_area();
}

void InputView::draw_button_area() {
    const float button_height = 32.0f;
    const float button_spacing = 8.0f;

    // 获取主题色
    auto& theme = DearTs::Core::UI::ThemeManager::instance();
    const ImVec4 accent_color = theme.getAccentColor();  // 强调色（蓝色）
    const ImVec4 link_color = theme.getColor("chat.link_color");  // Claude primary（橙色）

    // 获取可用宽度
    const float avail_width = ImGui::GetContentRegionAvail().x;

    // 计算按钮宽度（更紧凑）
    const float ai_button_width = 90.0f;
    const float send_button_width = 80.0f;
    const float total_buttons_width = ai_button_width + button_spacing + send_button_width;

    // 右对齐按钮
    const float start_x = avail_width - total_buttons_width;

    // AI 分析按钮（使用强调色）
    ImGui::SetCursorPosX(start_x);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, button_height / 2);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 4));
    ImGui::PushStyleColor(ImGuiCol_Button, accent_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
        accent_color.x * 1.15f, accent_color.y * 1.15f, accent_color.z * 1.15f, accent_color.w
    ));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(
        accent_color.x * 1.3f, accent_color.y * 1.3f, accent_color.z * 1.3f, accent_color.w
    ));

    const bool ai_analyze_clicked = ImGui::Button(ICON_AUTO_FIX_HIGH " AI", ImVec2(ai_button_width, button_height));

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    // AI 分析动画
    if (m_analyzing) {
        const float time = static_cast<float>(ImGui::GetTime());
        const float spin_offset = std::sin(time * 5.0f) * 3.0f;
        ImVec2 btn_min = ImGui::GetItemRectMin();
        ImGui::GetWindowDrawList()->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize() * 0.8f,
            ImVec2(btn_min.x + ai_button_width / 2 - ImGui::CalcTextSize(ICON_SYNC).x / 2 + spin_offset,
                  btn_min.y + button_height + 4),
            IM_COL32(100, 150, 255, 255),
            ICON_SYNC
        );
    }

    ImGui::SameLine(0.0f, button_spacing);

    // 发送按钮（使用 Claude primary 色）
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, button_height / 2);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 4));
    ImGui::PushStyleColor(ImGuiCol_Button, link_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
        link_color.x * 1.15f, link_color.y * 1.15f, link_color.z * 1.15f, link_color.w
    ));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(
        link_color.x * 1.3f, link_color.y * 1.3f, link_color.z * 1.3f, link_color.w
    ));

    const bool send_clicked = ImGui::Button(ICON_SEND " 发送", ImVec2(send_button_width, button_height));

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    // 处理按钮点击
    const double current_time = ImGui::GetTime();
    const bool cooldown_ok = (current_time - m_last_send_time) > 0.5;

    if (ai_analyze_clicked && cooldown_ok) {
        request_ai_analysis();
    }

    if (send_clicked && cooldown_ok) {
        send_message();
    }
}

void InputView::draw_suggestions() {
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

void InputView::clear_input() {
    m_input_buffer[0] = '\0';
}

void InputView::focus_input() {
    m_input_focused = true;
}

std::string InputView::get_input_text() const {
    return std::string(m_input_buffer);
}

void InputView::send_message() {
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

    // 发布消息发送事件（异步，避免阻塞 UI）
    DearTs::Core::Event::EventBus::instance().publish_async(Events::MessageSentEvent{
        .conversation_id = current_conv->id,
        .message = message
    });

    LOG_INFO("Sent message in conversation {}: {}", current_conv->id, content);

    // 自动触发 AI 分析请求（发送消息后立即获取 AI 响应，异步）
    DearTs::Core::Event::EventBus::instance().publish_async(Events::AIAnalysisRequestEvent{
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

void InputView::on_suggestion_clicked(const AISuggestion& suggestion) {
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

void InputView::request_ai_analysis() {
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

} // namespace DearTs::Plugins::Chat
