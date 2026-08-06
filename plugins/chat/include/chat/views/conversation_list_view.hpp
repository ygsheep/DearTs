/**
 * @file conversation_list_view.hpp
 * @brief 会话列表视图（左侧面板）
 */

#pragma once

#include "core/ui/view.h"
#include "core/ui/icon_font.hpp"
#include "core/event/event_bus.h"
#include "chat/models/conversation.hpp"
#include "chat/events/chat_events.hpp"
#include <memory>

namespace DearTs::Plugins::Chat {

using Core::ContentRegistry::UnlocalizedString;
using Core::UI::ViewWindow;

/**
 * @brief 会话列表视图
 * @details 显示所有会话，支持搜索、新建、删除等操作
 */
class ConversationListView : public ViewWindow {
public:
    explicit ConversationListView(std::shared_ptr<ConversationManager> manager)
        : ViewWindow(UnlocalizedString("会话"), ICON_MESSAGE)
        , m_conversation_manager(std::move(manager)) {
    }

    ~ConversationListView() override = default;

    void draw_content() override;
    ImVec2 get_min_size() const override { return ImVec2(250, 400); }

private:
    /**
     * @brief 绘制搜索栏
     */
    void draw_search_bar();

    /**
     * @brief 绘制会话列表
     */
    void draw_conversation_list();

    /**
     * @brief 绘制单个会话项
     */
    void draw_conversation_item(const std::shared_ptr<Conversation>& conv);

    /**
     * @brief 绘制右键菜单
     */
    void draw_context_menu(const std::shared_ptr<Conversation>& conv);

    /**
     * @brief 创建新会话
     */
    void create_new_conversation();

    /**
     * @brief 删除会话
     */
    void delete_conversation(const std::string& id);

    /**
     * @brief 重命名会话
     */
    void rename_conversation(const std::string& id);

    /**
     * @brief 切换到指定会话
     */
    void select_conversation(const std::shared_ptr<Conversation>& conv);

    /**
     * @brief 刷新会话列表
     */
    void refresh_list();

    // 成员变量
    std::shared_ptr<ConversationManager> m_conversation_manager;

    // 搜索
    char m_search_buffer[256] = "";
    std::string m_search_query;

    // 状态
    std::shared_ptr<Conversation> m_hovered_conversation;
    bool m_show_context_menu = false;

    // 重命名对话框状态
    bool m_show_rename_dialog = false;
    std::string m_rename_conversation_id;
    char m_rename_buffer[256] = "";

    // 事件订阅
    DearTs::Core::Event::EventToken m_conv_created_token;
    DearTs::Core::Event::EventToken m_conv_deleted_token;
    DearTs::Core::Event::EventToken m_conv_selected_token;
};

} // namespace DearTs::Plugins::Chat
