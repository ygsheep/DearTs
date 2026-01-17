/**
 * @file message_bubble.cpp
 * @brief 消息气泡组件实现
 */

#include "chat/ui/message_bubble.hpp"
#include "chat/ui/chat_theme.hpp"
#include "chat/ui/markdown_renderer.hpp"
#include "chat/ui/measure_context.hpp"
#include "core/ui/icon_font.hpp"
#include "logger.h"
#include <format>
#include <ctime>
#include <iomanip>
#include <functional>

namespace DearTs::Plugins::Chat::UI {
using namespace DearTs::Detail;
// const 版本的 draw 方法（仅用于不需要修改消息状态的用户/系统消息）
void MessageBubble::draw(const Message& message, const MessageBubbleStyle* style_ptr) {
    // 使用 ChatTheme 获取当前主题样式，如果没有提供自定义样式
    MessageBubbleStyle theme_style = ChatTheme::getMessageBubbleStyle();
    const MessageBubbleStyle& style = style_ptr ? *style_ptr : theme_style;

    switch (message.role) {
        case MessageRole::User:
            draw_user_message(message, style);
            break;
        case MessageRole::Assistant:
        case MessageRole::Other:
            // AI 消息需要非 const 引用以访问 expanded 状态
            // 注意：这里会修改 const 对象，调用方应确保这是安全的
            draw_ai_message(const_cast<Message&>(message), style);
            break;
        case MessageRole::System:
            draw_system_message(message, style);
            break;
    }
}

// 非 const 版本的 draw 方法（用于需要修改消息状态的场景）
void MessageBubble::draw(Message& message, const MessageBubbleStyle* style_ptr) {
    // 使用 ChatTheme 获取当前主题样式，如果没有提供自定义样式
    MessageBubbleStyle theme_style = ChatTheme::getMessageBubbleStyle();
    const MessageBubbleStyle& style = style_ptr ? *style_ptr : theme_style;

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
    const float avail_width = ImGui::GetContentRegionAvail().x;
    const float content_max_width = avail_width * style.max_width_percent;
    const float actual_max_width = std::min(content_max_width, style.max_width);
    const float right_margin = 20.0f;

    // 保存原始光标位置（相对坐标）
    ImVec2 original_cursor = ImGui::GetCursorPos();

    // 计算换行的绝对 X 坐标（相对于窗口内容区域）
    const float wrap_x = original_cursor.x + actual_max_width;

    // ===== 第一步：计算内容大小（不渲染） =====
    // 使用 CalcTextSize 而不是实际渲染，避免重复显示
    ImGui::PushTextWrapPos(wrap_x);
    ImVec2 content_size = ImGui::CalcTextSize(message.content.c_str(), nullptr, false, wrap_x - ImGui::GetCursorPos().x);
    ImGui::PopTextWrapPos();

    // ===== 第二步：计算气泡大小和位置 =====
    const ImVec2 bubble_size = ImVec2(
        content_size.x + style.padding_x * 2,
        content_size.y + style.padding_y * 2
    );

    // 计算右对齐偏移
    const float x_offset = avail_width - bubble_size.x - right_margin;

    // 恢复光标并获取屏幕位置
    ImGui::SetCursorPos(ImVec2(original_cursor.x + x_offset, original_cursor.y));
    ImVec2 base_screen_pos = ImGui::GetCursorScreenPos();
    const ImVec2 p_min = base_screen_pos;
    const ImVec2 p_max = ImVec2(p_min.x + bubble_size.x, p_min.y + bubble_size.y);

    // 检测悬停并选择边框颜色
    const bool is_hovered = style.enable_hover && ImGui::IsMouseHoveringRect(p_min, p_max);
    const ImVec4 border_color = is_hovered ? style.user_border_hover_color : style.user_border_color;

    // 绘制背景和边框
    draw_rounded_rect(p_min, p_max, style.user_corner_radius,
                      style.user_bg_color, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomLeft);

    if (style.draw_border) {
        draw_rounded_rect_border(p_min, p_max, style.user_corner_radius,
                                 border_color, style.user_border_width,
                                 ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomLeft);
    }

    // ===== 第三步：在正确位置渲染内容 =====
    ImGui::SetCursorPos(ImVec2(original_cursor.x + x_offset + style.padding_x, original_cursor.y + style.padding_y));
    // 用户消息右对齐，换行位置是右边界减去右边距和内边距
    const float user_wrap_x = original_cursor.x + avail_width - right_margin - style.padding_x;
    ImGui::PushTextWrapPos(user_wrap_x);
    ImGui::PushStyleColor(ImGuiCol_Text, style.user_text_color);
    ImGui::Text("%s", message.content.c_str());
    ImGui::PopStyleColor();
    ImGui::PopTextWrapPos();

    // ===== 第四步：绘制时间戳 =====
    if (style.show_timestamp) {
        // 使用屏幕坐标绘制时间戳
        ImVec2 timestamp_pos = ImVec2(p_max.x - 60.0f, p_max.y + style.spacing_y);

        // 转换为本地时间并格式化
        const auto time_tt = std::chrono::system_clock::to_time_t(message.timestamp);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time_tt);
#else
        localtime_r(&time_tt, &tm);
#endif
        char time_str[16];
        std::strftime(time_str, sizeof(time_str), "%H:%M", &tm);

        const ImU32 col = ImGui::ColorConvertFloat4ToU32(style.timestamp_color);
        ImGui::GetWindowDrawList()->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize() * 0.8f,
            timestamp_pos,
            col,
            time_str
        );

        // 绘制状态图标
        if (style.show_status) {
            draw_status_icon(message.status, ImVec2(p_max.x - 20.0f, p_max.y + style.spacing_y + 2));
        }
    }

    // ===== 第五步：设置下一个光标位置 =====
    const float next_y = original_cursor.y + bubble_size.y + style.spacing_y + 15;
    ImGui::SetCursorPos(ImVec2(original_cursor.x, next_y));
    // 添加 Dummy 来扩展窗口边界
    ImGui::Dummy(ImVec2(0, 0));
}

void MessageBubble::draw_ai_message(Message& message, const MessageBubbleStyle& style) {
    const float avail_width = ImGui::GetContentRegionAvail().x;
    // AI 消息宽度自适应：内容有多宽就显示多宽，但最大不超过父窗口的 90%
    const float max_limit_width = avail_width * 0.9f;

    // 计算最小宽度（时间戳 + 横线 + 按钮的最小空间需求）
    const float button_size = 24.0f;
    const float button_spacing = 4.0f;
    const float total_btn_width = button_size * 3 + button_spacing * 2;  // 3个按钮：copy、quote、code
    const float min_footer_width = 50.0f + 15.0f + 50.0f + 10.0f + total_btn_width + 20.0f;  // 时间戳 + 间距 + 最小横线 + 间距 + 按钮 + 左右边距
    const float min_content_width = min_footer_width + style.padding_x * 2;

    // 先测量内容的自然宽度（不换行）
    const ImVec2 text_size = ImGui::CalcTextSize(message.content.c_str(), nullptr, false, max_limit_width);
    const float content_natural_width = text_size.x + style.padding_x * 2;

    // 使用最大值：自然宽度、最大限制、最小宽度中的最大值
    const float actual_width = std::max({content_natural_width, min_content_width, 200.0f});  // 至少 200px

    const float left_margin = 20.0f;  // AI 消息左边距

    // 保存原始光标位置（相对坐标）
    ImVec2 original_cursor = ImGui::GetCursorPos();

    // ===== 使用 MeasureContext 测量高度（带缓存） =====
    MeasureStyle measure_style;
    measure_style.padding_x = style.padding_x;
    measure_style.padding_y = style.padding_y;
    measure_style.enable_markdown = style.enable_markdown;
    measure_style.text_color = style.ai_text_color;

    const float measured_content_height = MeasureContext::instance().measure_markdown_height(
        message.content,
        actual_width,
        measure_style
    );

    // ===== 直接使用测量高度渲染（只渲染一次） =====
    ImGui::SetCursorPos(ImVec2(original_cursor.x + left_margin, original_cursor.y));
    ImVec2 child_pos_before = ImGui::GetCursorScreenPos();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.padding_x, style.padding_y));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, style.ai_corner_radius);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, style.ai_bg_color);

    // 子窗口 ID（使用消息内容的哈希值来确保唯一性）
    const size_t content_hash = std::hash<std::string>{}(message.content);
    const auto time_hash = std::hash<decltype(message.timestamp.time_since_epoch().count())>{}(message.timestamp.time_since_epoch().count());
    const std::string render_name = "##ai_msg_render_" + std::to_string(content_hash) + "_" + std::to_string(time_hash);

    // 使用测量到的高度和自适应宽度
    ImGui::BeginChild(render_name.c_str(), ImVec2(actual_width, measured_content_height), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // 添加内边距
    ImGui::Indent(style.padding_x);
    ImGui::Dummy(ImVec2(0, style.padding_y));

    const float content_avail_width = actual_width - style.padding_x * 2;

    // 渲染内容（只渲染一次）
    if (style.enable_markdown) {
        MarkdownRenderer::render(message.content);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, style.ai_text_color);
        ImGui::PushTextWrapPos(content_avail_width);
        ImGui::Text("%s", message.content.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0, style.padding_y));
    ImGui::Unindent(style.padding_x);
    ImGui::Dummy(ImVec2(style.padding_x, 0));

    ImGui::EndChild();
    ImGui::PopStyleColor();  // ChildBg
    ImGui::PopStyleVar(3);    // WindowPadding, ChildRounding, ChildBorderSize

    // 获取子窗口实际大小
    ImVec2 child_size = ImGui::GetItemRectSize();

    // 绘制边框（在子窗口外部）
    ImVec2 p_min = child_pos_before;
    ImVec2 p_max = ImVec2(p_min.x + child_size.x, p_min.y + child_size.y);

    // 检测悬停并选择边框颜色
    const bool is_hovered = style.enable_hover && ImGui::IsMouseHoveringRect(p_min, p_max);
    const ImVec4 border_color = is_hovered ? style.ai_border_hover_color : style.ai_border_color;

    if (style.draw_border) {
        draw_rounded_rect_border(p_min, p_max, style.ai_corner_radius,
                                 border_color, style.ai_border_width,
                                 ImDrawFlags_RoundCornersTopRight | ImDrawFlags_RoundCornersBottomRight);
    }

    // 获取 DrawList（用于后续的时间戳和按钮绘制）
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // ===== 绘制时间戳和操作按钮 =====
    if (style.show_timestamp) {
        const float separator_y = p_max.y + style.spacing_y;
        const ImVec2 action_btn_size(24, 20);

        // 转换为本地时间
        const auto time_tt = std::chrono::system_clock::to_time_t(message.timestamp);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time_tt);
#else
        localtime_r(&time_tt, &tm);
#endif

        // 格式化时间
        char time_str[16];
        std::strftime(time_str, sizeof(time_str), "%H:%M", &tm);

        // 计算所需的最小宽度（时间戳 + 间距 + 最小横线 + 间距 + 按钮 + 右边距）
        // 使用函数开头已经声明的 button_spacing 和 total_btn_width
        const float min_line_width = 50.0f;  // 最小横线宽度

        // 计算时间戳文本高度，用于按钮垂直居中
        const float timestamp_text_height = ImGui::GetFontSize() * 0.8f;
        const float button_y_offset = (timestamp_text_height - action_btn_size.y) / 2.0f;  // 按钮向上偏移量

        // 绘制时间戳
        ImVec2 timestamp_pos = ImVec2(p_min.x + 10.0f, separator_y);
        const ImU32 timestamp_col = ImGui::ColorConvertFloat4ToU32(style.timestamp_color);
        draw_list->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize() * 0.8f,
            timestamp_pos,
            timestamp_col,
            time_str
        );

        // 计算时间戳后的横线起点和按钮位置
        const float timestamp_width = ImGui::CalcTextSize(time_str).x;
        const float time_right = timestamp_pos.x + timestamp_width;
        const float line_start_x = time_right + 15.0f;

        // 计算按钮位置（右对齐，3个按钮：copy、quote、code）
        const float action_btn_spacing = 4.0f;
        const float action_total_btn_width = action_btn_size.x * 3 + action_btn_spacing * 2;
        const float copy_btn_x = p_max.x - action_total_btn_width - 10.0f;  // 右边距 10px
        const float quote_btn_x = copy_btn_x + action_btn_size.x + action_btn_spacing;
        const float code_btn_x = quote_btn_x + action_btn_size.x + action_btn_spacing;  // code 在最右边

        // 绘制横线（从时间戳后到按钮前）
        const float line_end_x = copy_btn_x - 10.0f;
        const float actual_line_width = line_end_x - line_start_x;

        // 如果实际宽度太小，不绘制横线（避免挤在一起）
        if (actual_line_width > min_line_width) {
            const ImU32 line_col = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            draw_list->AddLine(
                ImVec2(line_start_x, separator_y + 10.0f),
                ImVec2(line_end_x, separator_y + 10.0f),
                line_col,
                1.0f
            );
        }

        // 按钮样式配置（使用主题色）
        const ImVec4 btn_bg = ChatTheme::getButtonBg();
        const ImVec4 btn_bg_hover = ChatTheme::getButtonHover();
        const ImVec4 btn_bg_active = ChatTheme::getButtonActive();
        const ImVec4 icon_color = ChatTheme::getButtonIcon();
        const ImVec4 icon_color_hover = ChatTheme::getButtonIconHover();
        const float btn_rounding = 4.0f;

        // 辅助函数：绘制圆角背景按钮
        auto draw_rounded_button = [&](const char* id, float x, const char* icon, const char* tooltip, std::function<void()> on_click = nullptr) {
            // 按钮位置：Y 轴偏移以与时间戳文本垂直居中
            ImVec2 btn_pos = ImVec2(x, separator_y + button_y_offset);
            ImGui::SetCursorScreenPos(btn_pos);

            // 检测悬停状态
            const ImVec2 mouse_pos = ImGui::GetMousePos();
            const ImVec2 btn_min = btn_pos;
            const ImVec2 btn_max = ImVec2(btn_pos.x + action_btn_size.x, btn_pos.y + action_btn_size.y);
            const bool is_hovered = ImGui::IsMouseHoveringRect(btn_min, btn_max);
            const bool is_clicked = is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            // 选择颜色
            const ImVec4 bg_col = is_clicked ? btn_bg_active : (is_hovered ? btn_bg_hover : btn_bg);
            const ImVec4 icon_col = is_hovered ? icon_color_hover : icon_color;

            // 绘制圆角背景
            draw_list->AddRectFilled(
                btn_min,
                btn_max,
                ImGui::ColorConvertFloat4ToU32(bg_col),
                btn_rounding
            );

            // 绘制图标（居中）
            const ImVec2 icon_size = ImGui::CalcTextSize(icon);
            const ImVec2 icon_pos = ImVec2(
                btn_min.x + (action_btn_size.x - icon_size.x) / 2,
                btn_min.y + (action_btn_size.y - icon_size.y) / 2
            );
            draw_list->AddText(
                ImGui::GetFont(),
                ImGui::GetFontSize() * 0.9f,
                icon_pos,
                ImGui::ColorConvertFloat4ToU32(icon_col),
                icon
            );

            // 添加 tooltip
            if (is_hovered && tooltip) {
                ImGui::SetTooltip("%s", tooltip);
            }

            // 处理点击
            if (is_clicked) {
                if (on_click) {
                    on_click();
                } else {
                    LOG_INFO("Button clicked: {}", id);
                }
            }
        };

        // 绘制复制按钮
        draw_rounded_button("##copy_ai", copy_btn_x, ICON_CONTENT_COPY, "复制消息");

        // 绘制引用按钮
        draw_rounded_button("##quote_ai", quote_btn_x, ICON_FORMAT_QUOTE, "引用消息");

        // 绘制 Code 按钮（打开/刷新 Markdown 窗口）
        draw_rounded_button("##code_expanded", code_btn_x, ICON_CODE, "打开完整 Markdown 视图", [&]() {
            // 设置为展开状态（如果未展开）
            if (!message.expanded) {
                const_cast<Message&>(message).expanded = true;
            }
            LOG_INFO("Markdown view opened/refreshed");
        });
    }

    // 如果消息已展开，渲染独立的 Markdown 窗口
    if (message.expanded) {
        render_expanded_markdown_window(const_cast<Message&>(message));
    }

    // ===== 设置下一个光标位置 =====
    const float next_y = original_cursor.y + child_size.y + style.spacing_y + (style.show_timestamp ? 30.0f : 0.0f);
    ImGui::SetCursorPos(ImVec2(original_cursor.x, next_y));
    // 添加 Dummy 来扩展窗口边界
    ImGui::Dummy(ImVec2(0, 0));

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

float MessageBubble::calc_markdown_height(const std::string& content, float width, float padding) {
    // 使用 MeasureContext（带缓存）
    MeasureStyle style;
    style.padding_x = padding;
    style.padding_y = padding;
    style.enable_markdown = true;

    return MeasureContext::instance().measure_markdown_height(content, width, style);
}

void MessageBubble::render_expanded_markdown_window(Message& message) {
    // 为每个消息生成唯一的窗口 ID
    const std::string window_id = "Expanded Markdown##" + message.id;

    // 设置窗口大小和位置（屏幕中央）
    const ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    const ImVec2 window_size(std::min(800.0f, screen_size.x * 0.8f), std::min(600.0f, screen_size.y * 0.8f));
    const ImVec2 window_pos((screen_size.x - window_size.x) * 0.5f, (screen_size.y - window_size.y) * 0.5f);

    ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);

    // 窗口标志
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin(window_id.c_str(), &message.expanded, window_flags)) {
        // 获取可用区域
        const ImVec2 avail_size = ImGui::GetContentRegionAvail();

        // 创建子窗口用于 Markdown 内容滚动
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.1f, 1.0f));

        if (ImGui::BeginChild("##expanded_content", avail_size, false, ImGuiWindowFlags_None)) {
            // 渲染 Markdown 内容
            MarkdownRenderer::render(message.content);
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

    ImGui::End();
}

} // namespace DearTs::Plugins::Chat::UI
