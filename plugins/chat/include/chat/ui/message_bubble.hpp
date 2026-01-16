/**
 * @file message_bubble.hpp
 * @brief 消息气泡组件
 */

#pragma once

#include "chat/models/message.hpp"
#include <string>
#include <imgui.h>

namespace DearTs::Plugins::Chat::UI {

/**
 * @brief 消息气泡样式配置
 */
struct MessageBubbleStyle {
    // 用户消息样式（右侧）
    ImVec4 user_bg_color = ImVec4(0.13f, 0.6f, 0.95f, 1.0f);
    ImVec4 user_text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 user_border_color = ImVec4(0.1f, 0.45f, 0.75f, 1.0f);      // 浅色边框
    ImVec4 user_border_hover_color = ImVec4(0.2f, 0.7f, 1.0f, 1.0f);   // 悬停高亮边框
    float user_corner_radius = 12.0f;
    float user_border_width = 1.5f;

    // AI 消息样式（左侧）
    ImVec4 ai_bg_color = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    ImVec4 ai_text_color = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    ImVec4 ai_border_color = ImVec4(0.25f, 0.25f, 0.3f, 1.0f);        // 浅色边框
    ImVec4 ai_border_hover_color = ImVec4(0.4f, 0.4f, 0.5f, 1.0f);     // 悬停高亮边框
    float ai_corner_radius = 12.0f;
    float ai_border_width = 1.5f;

    // 系统消息样式
    ImVec4 system_text_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

    // 时间戳样式
    ImVec4 timestamp_color = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);

    // 布局
    float max_width_percent = 0.8f;  // 文本宽度自适应 80%
    float max_width = 500.0f;        // 最大宽度限制
    float padding_x = 16.0f;
    float padding_y = 12.0f;
    float spacing_y = 8.0f;

    // 是否显示时间戳
    bool show_timestamp = true;

    // 是否显示状态图标
    bool show_status = true;

    // 是否绘制边框
    bool draw_border = true;

    // 是否启用悬停效果
    bool enable_hover = true;

    // Markdown 支持
    bool enable_markdown = true;       // 是否启用 Markdown 渲染（仅 AI 消息）
    bool monospace_code = true;        // 代码块使用等宽字体
};

/**
 * @brief 消息气泡组件
 * @details 绘制现代化的圆角消息气泡
 */
class MessageBubble {
public:
    /**
     * @brief 绘制消息气泡
     * @param message 消息对象
     * @param style 气泡样式（可选，使用默认样式）
     */
    static void draw(const Message& message, const MessageBubbleStyle* style = nullptr);

    /**
     * @brief 绘制用户消息（右侧）
     */
    static void draw_user_message(const Message& message, const MessageBubbleStyle& style);

    /**
     * @brief 绘制 AI 消息（左侧）
     */
    static void draw_ai_message(const Message& message, const MessageBubbleStyle& style);

    /**
     * @brief 绘制系统消息（居中）
     */
    static void draw_system_message(const Message& message, const MessageBubbleStyle& style);

    /**
     * @brief 绘制状态图标
     */
    static void draw_status_icon(MessageStatus status, ImVec2 pos);

    /**
     * @brief 绘制时间戳
     */
    static void draw_timestamp(const std::chrono::system_clock::time_point& timestamp,
                               const ImVec4& color);

    /**
     * @brief 计算消息气泡大小
     */
    static ImVec2 calc_size(const std::string& content, float max_width, const MessageBubbleStyle& style);

private:
    /**
     * @brief 绘制圆角矩形（填充）
     */
    static void draw_rounded_rect(const ImVec2& p_min, const ImVec2& p_max,
                                  float radius, const ImVec4& color, ImDrawFlags flags = 0);

    /**
     * @brief 绘制圆角矩形边框
     */
    static void draw_rounded_rect_border(const ImVec2& p_min, const ImVec2& p_max,
                                         float radius, const ImVec4& color,
                                         float width, ImDrawFlags flags = 0);

    /**
     * @brief 自动换行文本
     */
    static std::vector<std::string> wrap_text(const std::string& text, float max_width);
};

} // namespace DearTs::Plugins::Chat::UI
