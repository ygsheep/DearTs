/**
 * @file chat_view.hpp
 * @brief 聊天视图（中间面板）
 */

#pragma once

#include "core/ui/view.h"
#include "core/event/event_bus.h"
#include "chat/models/conversation.hpp"
#include "chat/models/ai_suggestion.hpp"
#include "chat/ui/message_bubble.hpp"
#include <memory>

namespace DearTs::Plugins::Chat {

using Core::ContentRegistry::UnlocalizedString;
using Core::UI::ViewWindow;

/**
 * @brief 聊天视图
 * @details 显示消息历史、输入框、AI 建议等
 */
class ChatView : public ViewWindow {
public:
    explicit ChatView(std::shared_ptr<ConversationManager> manager)
        : ViewWindow(UnlocalizedString("聊天"), ICON_CHAT)
        , m_conversation_manager(std::move(manager)) {
    }

    ~ChatView() override = default;

    void draw_content() override;
    ImVec2 get_min_size() const override { return ImVec2(400, 500); }

private:
    /**
     * @brief 绘制消息区域
     */
    void draw_message_area();

    /**
     * @brief 绘制 AI 建议区域
     */
    void draw_suggestions();

    /**
     * @brief 绘制输入区域
     */
    void draw_input_area();

    /**
     * @brief 绘制单个建议芯片
     */
    void draw_suggestion_chip(const AISuggestion& suggestion);

    /**
     * @brief 发送消息
     */
    void send_message();

    /**
     * @brief 处理 AI 建议点击
     */
    void on_suggestion_clicked(const AISuggestion& suggestion);

    /**
     * @brief 请求 AI 分析
     */
    void request_ai_analysis();

    /**
     * @brief 滚动到底部
     */
    void scroll_to_bottom();

    // 成员变量
    std::shared_ptr<ConversationManager> m_conversation_manager;

    // 输入
    char m_input_buffer[4096] = "";
    bool m_input_focused = false;

    // AI 建议
    std::vector<AISuggestion> m_suggestions;
    bool m_analyzing = false;
    bool m_show_suggestions = true;

    // 状态
    bool m_should_scroll_to_bottom = true;
    bool m_auto_scroll = true;

    // 消息气泡样式
    UI::MessageBubbleStyle m_bubble_style;

    // 事件订阅
    Event::EventToken m_message_sent_token;
    Event::EventToken m_message_received_token;
    Event::EventToken m_conv_selected_token;
    Event::EventToken m_suggestions_ready_token;
    Event::EventToken m_scroll_to_bottom_token;
};

} // namespace DearTs::Plugins::Chat
