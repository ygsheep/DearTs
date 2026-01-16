/**
 * @file message_bubble.cpp
 * @brief 消息气泡组件实现
 */

#include "chat/ui/message_bubble.hpp"
#include "chat/ui/markdown_renderer.hpp"
#include "core/ui/icon_font.hpp"
#include <format>
#include <ctime>
#include <iomanip>

namespace DearTs::Plugins::Chat::UI {

void MessageBubble::draw(const Message& message, const MessageBubbleStyle* style_ptr) {
    MessageBubbleStyle default_style;
    const MessageBubbleStyle& style = style_ptr ? *style_ptr : default_style;

    switch (message.role) {
        case MessageRole::User:
            draw_user_message(message, style);
            break;
        case MessageRole::Assistant:
        case MessageRole::Other:
            draw_ai_message(message, style);
            break;
        case MessageRole::System:
            draw_system_message(message, style);
            break;
    }
}

void MessageBubble::draw_user_message(const Message& message, const MessageBubbleStyle& style) {
    // 计算自适应宽度（80%）
    const float avail_width = ImGui::GetContentRegionAvail().x;
    const float content_max_width = avail_width * style.max_width_percent;
    const float actual_max_width = std::min(content_max_width, style.max_width);

    // 计算气泡大小
    const ImVec2 bubble_size = calc_size(message.content, actual_max_width, style);

    // 计算位置（右对齐）
    const float x_offset = avail_width - bubble_size.x - 20;

    // 绘制气泡背景
    ImVec2 base_pos = ImGui::GetCursorScreenPos();
    const ImVec2 p_min = ImVec2(base_pos.x + x_offset, base_pos.y);
    const ImVec2 p_max = ImVec2(p_min.x + bubble_size.x, p_min.y + bubble_size.y);

    // 检测悬停
    const bool is_hovered = style.enable_hover && ImGui::IsMouseHoveringRect(p_min, p_max);

    // 选择边框颜色
    const ImVec4 border_color = is_hovered ? style.user_border_hover_color : style.user_border_color;

    // 圆角矩形（左上和左下圆角，右上和右下直角 - 向右发送的效果）
    draw_rounded_rect(p_min, p_max, style.user_corner_radius,
                      style.user_bg_color, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomLeft);

    // 绘制边框
    if (style.draw_border) {
        draw_rounded_rect_border(p_min, p_max, style.user_corner_radius,
                                 border_color, style.user_border_width,
                                 ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomLeft);
    }

    // 绘制文本
    ImGui::SetCursorScreenPos(ImVec2(p_min.x + style.padding_x, p_min.y + style.padding_y));
    ImGui::PushTextWrapPos(p_min.x + bubble_size.x - style.padding_x);
    ImGui::PushStyleColor(ImGuiCol_Text, style.user_text_color);
    ImGui::Text("%s", message.content.c_str());
    ImGui::PopStyleColor();
    ImGui::PopTextWrapPos();

    // 绘制时间戳
    if (style.show_timestamp) {
        ImGui::SetCursorScreenPos(ImVec2(p_max.x - 60, p_max.y + style.spacing_y));
        draw_timestamp(message.timestamp, style.timestamp_color);

        // 绘制状态图标
        if (style.show_status) {
            draw_status_icon(message.status, ImVec2(p_max.x - 20, p_max.y + style.spacing_y + 2));
        }
    }

    // 设置下一个光标位置
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorStartPos().x, p_max.y + style.spacing_y + 15));
}

void MessageBubble::draw_ai_message(const Message& message, const MessageBubbleStyle& style) {
    // 计算自适应宽度（80%）
    const float avail_width = ImGui::GetContentRegionAvail().x;
    const float content_max_width = avail_width * style.max_width_percent;
    const float actual_max_width = std::min(content_max_width, style.max_width);

    // 计算气泡大小
    const ImVec2 bubble_size = calc_size(message.content, actual_max_width, style);

    // 绘制气泡背景
    ImVec2 base_pos = ImGui::GetCursorScreenPos();
    const ImVec2 p_min = ImVec2(base_pos.x + 20, base_pos.y);
    const ImVec2 p_max = ImVec2(p_min.x + bubble_size.x, p_min.y + bubble_size.y);

    // 检测悬停
    const bool is_hovered = style.enable_hover && ImGui::IsMouseHoveringRect(p_min, p_max);

    // 选择边框颜色
    const ImVec4 border_color = is_hovered ? style.ai_border_hover_color : style.ai_border_color;

    // 圆角矩形（右上和右下圆角，左上和左下直角 - 从左侧接收的效果）
    draw_rounded_rect(p_min, p_max, style.ai_corner_radius,
                      style.ai_bg_color, ImDrawFlags_RoundCornersTopRight | ImDrawFlags_RoundCornersBottomRight);

    // 绘制边框
    if (style.draw_border) {
        draw_rounded_rect_border(p_min, p_max, style.ai_corner_radius,
                                 border_color, style.ai_border_width,
                                 ImDrawFlags_RoundCornersTopRight | ImDrawFlags_RoundCornersBottomRight);
    }

    // 绘制文本
    ImGui::SetCursorScreenPos(ImVec2(p_min.x + style.padding_x, p_min.y + style.padding_y));
    ImGui::PushTextWrapPos(p_min.x + bubble_size.x - style.padding_x);

    // AI 消息支持 Markdown 渲染
    if (style.enable_markdown) {
        // Markdown 渲染器内部处理文本颜色
        MarkdownRenderer::render(message.content);
    } else {
        // 纯文本渲染
        ImGui::PushStyleColor(ImGuiCol_Text, style.ai_text_color);
        ImGui::Text("%s", message.content.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::PopTextWrapPos();

    // 绘制时间戳
    if (style.show_timestamp) {
        ImGui::SetCursorScreenPos(ImVec2(p_min.x + 10, p_max.y + style.spacing_y));
        draw_timestamp(message.timestamp, style.timestamp_color);
    }

    // 设置下一个光标位置
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorStartPos().x, p_max.y + style.spacing_y + 15));
}

void MessageBubble::draw_system_message(const Message& message, const MessageBubbleStyle& style) {
    ImGui::PushStyleColor(ImGuiCol_Text, style.system_text_color);
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 2 - ImGui::CalcTextSize(message.content.c_str()).x / 2);
    ImGui::Text("--- %s ---", message.content.c_str());
    ImGui::PopStyleColor();

    // 添加间距
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
}

void MessageBubble::draw_status_icon(MessageStatus status, ImVec2 pos) {
    const char* icon = nullptr;
    ImVec4 color;

    switch (status) {
        case MessageStatus::Sent:
            icon = ICON_DONE_ALL;  // 双勾
            color = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
            break;
        case MessageStatus::Sending:
            icon = ICON_SYNC;  // 同步图标
            color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            break;
        case MessageStatus::Failed:
            icon = ICON_ERROR;  // 错误图标
            color = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
            break;
        default:
            return;
    }

    ImGui::GetWindowDrawList()->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize() * 0.8f,
        pos,
        IM_COL32(
            static_cast<int>(color.x * 255),
            static_cast<int>(color.y * 255),
            static_cast<int>(color.z * 255),
            static_cast<int>(color.w * 255)
        ),
        icon
    );
}

void MessageBubble::draw_timestamp(const std::chrono::system_clock::time_point& timestamp,
                                    const ImVec4& color) {
    // 转换为本地时间
    const auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif

    // 格式化时间
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M");
    const std::string time_str = oss.str();

    // 绘制时间戳
    ImGui::PushFont(ImGui::GetFont());
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("%s", time_str.c_str());
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

ImVec2 MessageBubble::calc_size(const std::string& content, float max_width, const MessageBubbleStyle& style) {
    // 计算文本大小
    ImVec2 text_size = ImGui::CalcTextSize(content.c_str(), nullptr, false, max_width - style.padding_x * 2);

    // 加上内边距
    return ImVec2(
        text_size.x + style.padding_x * 2,
        text_size.y + style.padding_y * 2
    );
}

void MessageBubble::draw_rounded_rect(const ImVec2& p_min, const ImVec2& p_max,
                                       float radius, const ImVec4& color, ImDrawFlags flags) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImU32 col = ImGui::ColorConvertFloat4ToU32(color);

    draw_list->AddRectFilled(p_min, p_max, col, radius, flags);
}

void MessageBubble::draw_rounded_rect_border(const ImVec2& p_min, const ImVec2& p_max,
                                              float radius, const ImVec4& color,
                                              float width, ImDrawFlags flags) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImU32 col = ImGui::ColorConvertFloat4ToU32(color);

    draw_list->AddRect(p_min, p_max, col, radius, flags, width);
}

std::vector<std::string> MessageBubble::wrap_text(const std::string& text, float max_width) {
    std::vector<std::string> lines;
    // ImGui 会自动处理换行，这里简化处理
    lines.push_back(text);
    return lines;
}

} // namespace DearTs::Plugins::Chat::UI
