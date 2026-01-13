/**
 * @file conversation_list_view.cpp
 * @brief 会话列表视图实现
 */

#include "chat/views/conversation_list_view.hpp"
#include "liblogger/logger.h"
#include <imgui.h>
#include <fmt/format.h>

namespace DearTs::Plugins::Chat {

void ConversationListView::draw_content() {
    // 搜索栏
    draw_search_bar();

    ImGui::Separator();

    // 会话列表
    draw_conversation_list();

    // 右键菜单
    if (m_show_context_menu && ImGui::BeginPopupContextWindow("ConversationContextMenu")) {
        if (ImGui::MenuItem("新建会话", "Ctrl+N")) {
            create_new_conversation();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("刷新", "F5")) {
            refresh_list();
        }
        ImGui::EndPopup();
    }
}

void ConversationListView::draw_search_bar() {
    // 搜索输入框
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));
    if (ImGui::InputTextWithHint(
        "##search",
        ICON_SEARCH " 搜索会话...",
        m_search_buffer,
        sizeof(m_search_buffer),
        ImGuiInputTextFlags_EnterReturnsTrue
    )) {
        m_search_query = m_search_buffer;
        refresh_list();
    }
    ImGui::PopStyleColor();

    // 清空搜索按钮
    if (!m_search_query.empty() && ImGui::IsItemHovered()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            m_search_query.clear();
            m_search_buffer[0] = '\0';
            refresh_list();
        }
    }
}

void ConversationListView::draw_conversation_list() {
    auto& conversations = m_conversation_manager->get_conversations();

    // 新建会话按钮
    if (ImGui::Button("+ 新建会话", ImVec2(-1, 0))) {
        create_new_conversation();
    }

    ImGui::Separator();

    // 如果没有会话
    if (conversations.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);
        ImGui::TextWrapped("暂无会话");
        ImGui::TextWrapped("点击上方按钮创建新会话");
        ImGui::PopStyleColor();
        return;
    }

    // 绘制会话列表（使用虚拟滚动）
    ImGuiListClipper clipper;
    clipper.Begin(conversations.size());

    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const auto& conv = conversations[i];

            // 过滤搜索
            if (!m_search_query.empty()) {
                const auto& title = conv->get_display_title();
                if (title.find(m_search_query) == std::string::npos) {
                    continue;
                }
            }

            draw_conversation_item(conv);
        }
    }
    clipper.End();
}

void ConversationListView::draw_conversation_item(const std::shared_ptr<Conversation>& conv) {
    // 判断是否为当前选中的会话
    const bool is_selected = (m_conversation_manager->get_current_conversation() &&
                              m_conversation_manager->get_current_conversation()->id == conv->id);

    // 判断是否悬停
    const bool is_hovered = (m_hovered_conversation && m_hovered_conversation->id == conv->id);

    // 选择颜色
    if (is_selected) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.13f, 0.13f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
    } else if (is_hovered) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.08f, 0.08f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.05f, 0.05f, 0.07f, 1.0f));
    }

    // 会话项
    const auto& title = conv->get_display_title();
    const auto& preview = conv->get_last_message_preview();
    const auto current_conv = m_conversation_manager->get_current_conversation();

    if (ImGui::Selectable(fmt::format("##{}", conv->id).c_str(), is_selected, ImGuiSelectableFlags_None, ImVec2(0, 60))) {
        select_conversation(conv);
    }

    // 检测悬停
    if (ImGui::IsItemHovered()) {
        m_hovered_conversation = conv;
    } else if (m_hovered_conversation && m_hovered_conversation->id == conv->id && !ImGui::IsItemHovered()) {
        m_hovered_conversation.reset();
    }

    // 右键菜单
    if (ImGui::BeginPopupContextItem(fmt::format("context_{}", conv->id).c_str())) {
        if (ImGui::MenuItem("删除会话")) {
            delete_conversation(conv->id);
        }
        if (conv->is_pinned) {
            if (ImGui::MenuItem("取消置顶")) {
                conv->is_pinned = false;
            }
        } else {
            if (ImGui::MenuItem("置顶")) {
                conv->is_pinned = true;
            }
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(2);

    // 绘制内容（在 Selectable 之后）
    ImGui::SameLine(0, 0);

    // 图标
    const char* icon = ICON_MESSAGE;
    if (conv->type == ConversationType::AI) {
        icon = ICON_SMART_TOY;
    } else if (conv->type == ConversationType::Group) {
        icon = ICON_GROUP;
    }

    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorStartPos().x + 10, ImGui::GetCursorStartPos().y + 10));
    ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "%s", icon);

    // 标题
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorStartPos().x + 35, ImGui::GetCursorStartPos().y + 8));

    if (conv->unread_count > 0) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", title.c_str());
        ImGui::SameLine();

        // 未读徽章
        const std::string badge = fmt::format(" {}", conv->unread_count);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 5);
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "%s", badge.c_str());
    } else {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", title.c_str());
    }

    // 预览文本
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorStartPos().x + 35, ImGui::GetCursorStartPos().y + 30));

    if (!preview.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", preview.c_str());
    } else {
        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "暂无消息");
    }

    // 时间戳
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorStartPos().x + 10, ImGui::GetCursorStartPos().y + 42));

    const auto now = std::chrono::system_clock::now();
    const auto diff = std::chrono::duration_cast<std::chrono::hours>(now - conv->updated_at).count();

    std::string time_str;
    if (diff < 1) {
        time_str = "刚刚";
    } else if (diff < 24) {
        time_str = fmt::format("{}小时前", diff);
    } else if (diff < 24 * 7) {
        time_str = fmt::format("{}天前", diff / 24);
    } else {
        time_str = fmt::format("{:%m-%d}", conv->updated_at);
    }

    ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "%s", time_str.c_str());

    // 置顶标记
    if (conv->is_pinned) {
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "%s", ICON_PUSH_PIN);
    }
}

void ConversationListView::draw_context_menu(const std::shared_ptr<Conversation>& conv) {
    // 右键菜单已在 draw_conversation_item 中处理
}

void ConversationListView::create_new_conversation() {
    auto conv = m_conversation_manager->create_conversation("新对话", ConversationType::AI);
    if (conv) {
        LOG_INFO("Created new conversation: {}", conv->id);
        select_conversation(conv);
    }
}

void ConversationListView::delete_conversation(const std::string& id) {
    if (m_conversation_manager->delete_conversation(id)) {
        LOG_INFO("Deleted conversation: {}", id);
    }
}

void ConversationListView::select_conversation(const std::shared_ptr<Conversation>& conv) {
    m_conversation_manager->set_current_conversation(conv);
    conv->unread_count = 0;

    // 发布事件
    EventBus::instance().publish(Events::ConversationSelectedEvent{ conv });
}

void ConversationListView::refresh_list() {
    // 刷新逻辑（如果需要从存储加载）
}

} // namespace DearTs::Plugins::Chat
