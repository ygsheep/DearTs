/**
 * @file input_view.hpp
 * @brief 输入视图（独立窗口）
 */

#pragma once

#include "core/ui/view.h"
#include "core/ui/icon_font.hpp"
#include "core/event/event_bus.h"
#include "chat/models/conversation.hpp"
#include "chat/models/ai_suggestion.hpp"
#include <memory>
#include <vector>

namespace DearTs::Plugins::Chat {

using Core::ContentRegistry::UnlocalizedString;
using Core::UI::ViewWindow;

/**
 * @brief 输入视图
 * @details 独立的输入窗口，包含输入框和发送按钮
 */
class InputView : public ViewWindow {
public:
    explicit InputView(std::shared_ptr<ConversationManager> manager)
        : ViewWindow(UnlocalizedString("输入"), ICON_EDIT)
        , m_conversation_manager(std::move(manager)) {
    }

    ~InputView() override = default;

    void draw_content() override;
    ImVec2 get_min_size() const override { return ImVec2(400, 100); }

    /**
     * @brief 清空输入框
     */
    void clear_input();

    /**
     * @brief 设置输入框聚焦
     */
    void focus_input();

    /**
     * @brief 获取输入框内容
     */
    std::string get_input_text() const;

private:
    /**
     * @brief 绘制输入区域
     */
    void draw_input_area();

    /**
     * @brief 绘制预览区域
     */
    void draw_preview_area();

    /**
     * @brief 绘制按钮区域
     */
    void draw_button_area();

    /**
     * @brief 绘制 AI 建议区域
     */
    void draw_suggestions();

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

    // 成员变量
    std::shared_ptr<ConversationManager> m_conversation_manager;

    // 输入
    char m_input_buffer[4096] = "";
    bool m_input_focused = false;
    bool m_enter_was_down = false;  // 跟踪 Enter 键上一帧状态

    // AI 建议
    std::vector<AISuggestion> m_suggestions;
    bool m_analyzing = false;
    bool m_show_suggestions = true;

    // 状态
    bool m_sending = false;  // 防止重复发送
    double m_last_send_time = 0.0;  // 上次发送时间（秒）

    // 事件订阅
    DearTs::Core::Event::EventToken m_suggestions_ready_token;
};

} // namespace DearTs::Plugins::Chat
