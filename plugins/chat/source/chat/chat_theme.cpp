/**
 * @file chat_theme.cpp
 * @brief ChatManager 主题辅助类实现
 */

#include "chat/ui/chat_theme.hpp"
#include "core/ui/theme_manager.h"
#include <algorithm>

namespace DearTs::Plugins::Chat::UI {

MessageBubbleStyle ChatTheme::getMessageBubbleStyle() {
    MessageBubbleStyle style;

    // 从 ThemeManager 获取颜色
    style.user_bg_color = getUserMessageBg();
    style.user_text_color = getUserMessageText();
    style.user_border_color = getUserMessageBorder();
    style.user_border_hover_color = getUserBorderHover();

    style.ai_bg_color = getAIMessageBg();
    style.ai_text_color = getAIMessageText();
    style.ai_border_color = getAIMessageBorder();
    style.ai_border_hover_color = getAIBorderHover();

    style.system_text_color = getSystemText();
    style.timestamp_color = getTimestamp();

    // 减淡背景色 15%（使颜色更柔和）
    style.user_bg_color = lightenColor(style.user_bg_color, 0.15f);
    style.ai_bg_color = lightenColor(style.ai_bg_color, 0.15f);

    // 从 ThemeManager 获取布局参数
    float radius = getBorderRadius();
    style.user_corner_radius = radius;
    style.ai_corner_radius = radius;

    // 保持其他默认布局参数
    style.max_width_percent = 0.8f;
    style.max_width = 500.0f;
    style.padding_x = 16.0f;
    style.padding_y = 12.0f;
    style.spacing_y = 8.0f;
    style.user_border_width = 1.5f;
    style.ai_border_width = 1.5f;

    // 默认功能开关
    style.show_timestamp = true;
    style.show_status = true;
    style.draw_border = true;
    style.enable_hover = true;
    style.enable_markdown = true;
    style.monospace_code = true;

    return style;
}

ImVec4 ChatTheme::lightenColor(const ImVec4& color, float amount) {
    return ImVec4(
        std::min(color.x + amount, 1.0f),
        std::min(color.y + amount, 1.0f),
        std::min(color.z + amount, 1.0f),
        color.w
    );
}

ImVec4 ChatTheme::darkenColor(const ImVec4& color, float amount) {
    return ImVec4(
        std::max(color.x - amount, 0.0f),
        std::max(color.y - amount, 0.0f),
        std::max(color.z - amount, 0.0f),
        color.w
    );
}

ImVec4 ChatTheme::withAlpha(const ImVec4& color, float alpha) {
    return ImVec4(color.x, color.y, color.z, std::clamp(alpha, 0.0f, 1.0f));
}

} // namespace DearTs::Plugins::Chat::UI
