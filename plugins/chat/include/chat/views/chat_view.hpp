/**
 * @file chat_view.hpp
 * @brief 聊天视图（消息显示区域）
 */

#pragma once

#include "core/ui/view.h"
#include "core/ui/icon_font.hpp"
#include "core/event/event_bus.h"
#include "chat/models/conversation.hpp"
#include "chat/ui/message_bubble.hpp"
#include <memory>

namespace DearTs::Plugins::Chat {

using Core::ContentRegistry::UnlocalizedString;
using Core::UI::ViewWindow;

/**
 * @brief 聊天视图
 * @details 显示消息历史，支持滚动和虚拟滚动优化
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
     * @brief 滚动到底部
     */
    void scroll_to_bottom();

    // 成员变量
    std::shared_ptr<ConversationManager> m_conversation_manager;

    // 状态
    bool m_should_scroll_to_bottom = true;
    bool m_auto_scroll = true;

    // 消息气泡样式
    UI::MessageBubbleStyle m_bubble_style;

    // 事件订阅
    DearTs::Core::Event::EventToken m_message_sent_token;
    DearTs::Core::Event::EventToken m_message_received_token;
    DearTs::Core::Event::EventToken m_conv_selected_token;
    DearTs::Core::Event::EventToken m_scroll_to_bottom_token;
};

} // namespace DearTs::Plugins::Chat
