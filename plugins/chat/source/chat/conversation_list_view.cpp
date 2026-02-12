/**
 * @file conversation_list_view.cpp
 * @brief 会话列表视图实现
 */

#include "chat/views/conversation_list_view.hpp"
#include "core/ui/icon_font.hpp"
#include "liblogger/logger.h"
#include <imgui.h>
#include <format>

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

    // ✅ 重命名对话框
    if (m_show_rename_dialog) {
        ImGui::OpenPopup("重命名会话");
        m_show_rename_dialog = false;  // 只触发一次
    }

    if (ImGui::BeginPopupModal("重命名会话", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("新标题:");
        ImGui::InputText("##new_title", m_rename_buffer, sizeof(m_rename_buffer));

        bool confirm = false;
        if (ImGui::Button("确定", ImVec2(120, 0))) {
            confirm = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        // 按 Enter 键确认
        if (ImGui::IsItemFocused() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))) {
            confirm = true;
        }

        if (confirm) {
            std::string new_title(m_rename_buffer);
            if (!new_title.empty() && new_title != m_rename_conversation_id) {
                rename_conversation(m_rename_conversation_id);
            }
            ImGui::CloseCurrentPopup();
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

    if (ImGui::Selectable(std::format("##{}", conv->id).c_str(), is_selected, ImGuiSelectableFlags_None, ImVec2(0, 60))) {
        select_conversation(conv);
    }

    // 保存 Selectable 的位置和大小
    const ImVec2 item_min = ImGui::GetItemRectMin();
    const ImVec2 item_size = ImGui::GetItemRectSize();

    // 检测悬停
    if (ImGui::IsItemHovered()) {
        m_hovered_conversation = conv;
    } else if (m_hovered_conversation && m_hovered_conversation->id == conv->id && !ImGui::IsItemHovered()) {
        m_hovered_conversation.reset();
    }

    // 右键菜单
    if (ImGui::BeginPopupContextItem(std::format("context_{}", conv->id).c_str())) {
        if (ImGui::MenuItem("重命名")) {
            // 打开重命名对话框
            m_show_rename_dialog = true;
            m_rename_conversation_id = conv->id;
            // 初始化重命名缓冲区为当前标题
            strncpy_s(m_rename_buffer, sizeof(m_rename_buffer), conv->title.c_str(), _TRUNCATE);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("删除会话")) {
            // 删除会话后立即返回，避免继续访问已删除的数据
            delete_conversation(conv->id);
            // 关闭弹出菜单，避免后续代码访问已删除的会话
            ImGui::CloseCurrentPopup();
            return;
        }
        ImGui::Separator();
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

    // 获取窗口绘制列表
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 图标
    const char* icon = ICON_MESSAGE;
    if (conv->type == ConversationType::AI) {
        icon = ICON_SMART_TOY;
    } else if (conv->type == ConversationType::Group) {
        icon = ICON_GROUP;
    }

    const ImVec2 icon_pos(item_min.x + 10, item_min.y + 10);
    draw_list->AddText(icon_pos, IM_COL32(100, 150, 255, 255), icon);

    // 限制文本宽度
    const float max_text_width = item_size.x - 55;

    // 标题（截断过长的标题）
    const ImVec2 title_pos(item_min.x + 35, item_min.y + 8);
    std::string display_title = title;
    const ImVec2 title_size = ImGui::CalcTextSize(title.c_str());

    if (title_size.x > max_text_width) {
        // 二分查找截断点
        size_t left = 0, right = title.length();
        while (left < right) {
            size_t mid = (left + right + 1) / 2;
            std::string test = title.substr(0, mid) + "…";
            if (ImGui::CalcTextSize(test.c_str()).x <= max_text_width) {
                left = mid;
            } else {
                right = mid - 1;
            }
        }
        display_title = title.substr(0, left) + "…";
    }

    // 绘制标题
    ImU32 title_color = conv->unread_count > 0 ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 255);
    draw_list->AddText(title_pos, title_color, display_title.c_str());

    // 未读徽章
    if (conv->unread_count > 0) {
        const std::string badge = std::format(" {}", conv->unread_count);
        const ImVec2 badge_pos(title_pos.x + ImGui::CalcTextSize(display_title.c_str()).x - 5, title_pos.y);
        draw_list->AddText(badge_pos, IM_COL32(50, 150, 255, 255), badge.c_str());
    }

    // 预览文本（截断）
    const ImVec2 preview_pos(item_min.x + 35, item_min.y + 30);
    std::string display_preview;

    if (!preview.empty()) {
        display_preview = preview;
        if (display_preview.length() > 50) {
            display_preview = preview.substr(0, 50) + "…";
        }

        // 进一步截断以适应宽度
        const ImVec2 preview_size = ImGui::CalcTextSize(display_preview.c_str());
        if (preview_size.x > max_text_width) {
            size_t left = 0, right = display_preview.length();
            while (left < right) {
                size_t mid = (left + right + 1) / 2;
                std::string test = display_preview.substr(0, mid) + "…";
                if (ImGui::CalcTextSize(test.c_str()).x <= max_text_width) {
                    left = mid;
                } else {
                    right = mid - 1;
                }
            }
            display_preview = display_preview.substr(0, left) + "…";
        }
    } else {
        display_preview = "暂无消息";
    }

    draw_list->AddText(preview_pos, IM_COL32(128, 128, 128, 255), display_preview.c_str());

    // 时间戳
    const auto now = std::chrono::system_clock::now();
    const auto diff = std::chrono::duration_cast<std::chrono::hours>(now - conv->updated_at).count();

    std::string time_str;
    if (diff < 1) {
        time_str = "刚刚";
    } else if (diff < 24) {
        time_str = std::format("{}小时前", diff);
    } else if (diff < 24 * 7) {
        time_str = std::format("{}天前", diff / 24);
    } else {
        time_str = std::format("{:%m-%d}", conv->updated_at);
    }

    const ImVec2 time_pos(item_min.x + 10, item_min.y + 42);
    draw_list->AddText(time_pos, IM_COL32(128, 128, 128, 255), time_str.c_str());

    // 置顶标记
    if (conv->is_pinned) {
        const ImVec2 pin_pos(item_min.x + item_size.x - 25, item_min.y + 10);
        draw_list->AddText(pin_pos, IM_COL32(200, 150, 50, 255), ICON_PUSH_PIN);
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

void ConversationListView::rename_conversation(const std::string& id) {
    if (m_conversation_manager->rename_conversation(id, std::string(m_rename_buffer))) {
        LOG_INFO("Renamed conversation: {}", id);
    } else {
        LOG_WARN("Failed to rename conversation: {}", id);
    }
}

void ConversationListView::select_conversation(const std::shared_ptr<Conversation>& conv) {
    m_conversation_manager->set_current_conversation(conv);
    conv->unread_count = 0;

    // 发布事件
    DearTs::Core::Event::EventBus::instance().publish(Events::ConversationSelectedEvent{ conv });
}

void ConversationListView::refresh_list() {
    // 刷新逻辑（如果需要从存储加载）
}

} // namespace DearTs::Plugins::Chat
