/**
 * @file chat_theme.hpp
 * @brief ChatManager 主题辅助类
 * @details 提供统一的主题颜色访问接口，集成 ThemeManager
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "chat/ui/message_bubble.hpp"
#include "core/ui/theme_manager.h"
#include <imgui.h>
#include <string>

namespace DearTs::Plugins::Chat::UI {

/**
 * @brief ChatManager 主题辅助类
 * @details 提供静态方法访问 ThemeManager 中的聊天主题颜色
 *
 * 所有聊天 UI 组件应通过此类获取颜色，而不是直接使用硬编码颜色值。
 * 这样可以实现主题切换时自动更新所有组件的颜色。
 */
class ChatTheme {
public:
    // ==================== 消息气泡颜色 ====================

    /**
     * @brief 获取消息气泡样式
     * @return 包含当前主题颜色的 MessageBubbleStyle
     */
    [[nodiscard]] static MessageBubbleStyle getMessageBubbleStyle();

    // ==================== 用户消息颜色 ====================

    [[nodiscard]] static ImVec4 getUserMessageBg();
    [[nodiscard]] static ImVec4 getUserMessageText();
    [[nodiscard]] static ImVec4 getUserMessageBorder();
    [[nodiscard]] static ImVec4 getUserBorderHover();

    // ==================== AI 消息颜色 ====================

    [[nodiscard]] static ImVec4 getAIMessageBg();
    [[nodiscard]] static ImVec4 getAIMessageText();
    [[nodiscard]] static ImVec4 getAIMessageBorder();
    [[nodiscard]] static ImVec4 getAIBorderHover();

    // ==================== 输入框颜色 ====================

    [[nodiscard]] static ImVec4 getInputBg();
    [[nodiscard]] static ImVec4 getInputBorder();
    [[nodiscard]] static ImVec4 getInputFocus();
    [[nodiscard]] static ImVec4 getInputPlaceholder();
    [[nodiscard]] static ImVec4 getInputBorder(bool focused);

    // ==================== 建议芯片颜色 ====================

    [[nodiscard]] static ImVec4 getSuggestionBg();
    [[nodiscard]] static ImVec4 getSuggestionHover();
    [[nodiscard]] static ImVec4 getSuggestionText();
    [[nodiscard]] static ImVec4 getSuggestionBg(bool hovered);

    // ==================== 代码块颜色 ====================

    [[nodiscard]] static ImVec4 getCodeBlockBg();
    [[nodiscard]] static ImVec4 getCodeBlockText();

    // ==================== 通用颜色 ====================

    [[nodiscard]] static ImVec4 getChatBackground();
    [[nodiscard]] static ImVec4 getSurfaceColor();
    [[nodiscard]] static ImVec4 getSurfaceHighlight();
    [[nodiscard]] static ImVec4 getSystemText();
    [[nodiscard]] static ImVec4 getTimestamp();
    [[nodiscard]] static ImVec4 getLinkColor();
    [[nodiscard]] static ImVec4 getErrorBg();
    [[nodiscard]] static ImVec4 getErrorText();

    // ==================== 按钮颜色 ====================

    [[nodiscard]] static ImVec4 getButtonBg();
    [[nodiscard]] static ImVec4 getButtonHover();
    [[nodiscard]] static ImVec4 getButtonActive();
    [[nodiscard]] static ImVec4 getButtonIcon();
    [[nodiscard]] static ImVec4 getButtonIconHover();

    // ==================== 布局参数 ====================

    /**
     * @brief 获取默认圆角半径
     */
    [[nodiscard]] static float getBorderRadius() {
        return ::DearTs::Core::UI::ThemeManager::instance().getBorderRadius();
    }

    /**
     * @brief 获取玻璃态透明度
     */
    [[nodiscard]] static float getGlassAlpha() {
        return ::DearTs::Core::UI::ThemeManager::instance().getGlassAlpha();
    }

    /**
     * @brief 获取强调色
     */
    [[nodiscard]] static ImVec4 getAccentColor() {
        return ::DearTs::Core::UI::ThemeManager::instance().getAccentColor();
    }

    // ==================== 工具函数 ====================

    /**
     * @brief 使颜色变亮（用于悬停效果）
     * @param color 原始颜色
     * @param amount 变亮量 (0.0 - 1.0)
     * @return 变亮后的颜色
     */
    [[nodiscard]] static ImVec4 lightenColor(const ImVec4& color, float amount = 0.1f);

    /**
     * @brief 使颜色变暗（用于按下效果）
     * @param color 原始颜色
     * @param amount 变暗量 (0.0 - 1.0)
     * @return 变暗后的颜色
     */
    [[nodiscard]] static ImVec4 darkenColor(const ImVec4& color, float amount = 0.1f);

    /**
     * @brief 调整颜色透明度
     * @param color 原始颜色
     * @param alpha 新的透明度 (0.0 - 1.0)
     * @return 调整透明度后的颜色
     */
    [[nodiscard]] static ImVec4 withAlpha(const ImVec4& color, float alpha);

private:
    // 内部辅助函数：从 ThemeManager 获取颜色
    [[nodiscard]] static ImVec4 getColor(const char* key);
};

// ==================== 内联实现 ====================

inline ImVec4 ChatTheme::getUserMessageBg() {
    return getColor("chat.user_message_bg");
}

inline ImVec4 ChatTheme::getUserMessageText() {
    return getColor("chat.user_message_text");
}

inline ImVec4 ChatTheme::getUserMessageBorder() {
    return getColor("chat.user_message_border");
}

inline ImVec4 ChatTheme::getUserBorderHover() {
    return getColor("chat.user_border_hover");
}

inline ImVec4 ChatTheme::getAIMessageBg() {
    return getColor("chat.ai_message_bg");
}

inline ImVec4 ChatTheme::getAIMessageText() {
    return getColor("chat.ai_message_text");
}

inline ImVec4 ChatTheme::getAIMessageBorder() {
    return getColor("chat.ai_message_border");
}

inline ImVec4 ChatTheme::getAIBorderHover() {
    return getColor("chat.ai_border_hover");
}

inline ImVec4 ChatTheme::getInputBg() {
    return getColor("chat.input_bg");
}

inline ImVec4 ChatTheme::getInputBorder() {
    return getColor("chat.input_border");
}

inline ImVec4 ChatTheme::getInputFocus() {
    return getColor("chat.input_focus");
}

inline ImVec4 ChatTheme::getInputPlaceholder() {
    return getColor("chat.input_placeholder");
}

inline ImVec4 ChatTheme::getInputBorder(bool focused) {
    return focused ? getInputFocus() : getInputBorder();
}

inline ImVec4 ChatTheme::getSuggestionBg() {
    return getColor("chat.suggestion_bg");
}

inline ImVec4 ChatTheme::getSuggestionHover() {
    return getColor("chat.suggestion_hover");
}

inline ImVec4 ChatTheme::getSuggestionText() {
    return getColor("chat.suggestion_text");
}

inline ImVec4 ChatTheme::getSuggestionBg(bool hovered) {
    return hovered ? getSuggestionHover() : getSuggestionBg();
}

inline ImVec4 ChatTheme::getCodeBlockBg() {
    return getColor("chat.code_bg");
}

inline ImVec4 ChatTheme::getCodeBlockText() {
    return getColor("chat.code_text");
}

inline ImVec4 ChatTheme::getChatBackground() {
    return getColor("chat.background");
}

inline ImVec4 ChatTheme::getSurfaceColor() {
    return getColor("chat.surface");
}

inline ImVec4 ChatTheme::getSurfaceHighlight() {
    return getColor("chat.surface_highlight");
}

inline ImVec4 ChatTheme::getSystemText() {
    return getColor("chat.system_text");
}

inline ImVec4 ChatTheme::getTimestamp() {
    return getColor("chat.timestamp");
}

inline ImVec4 ChatTheme::getLinkColor() {
    return getColor("chat.link_color");
}

inline ImVec4 ChatTheme::getErrorBg() {
    return getColor("chat.error_bg");
}

inline ImVec4 ChatTheme::getErrorText() {
    return getColor("chat.error_text");
}

inline ImVec4 ChatTheme::getButtonBg() {
    return getColor("chat.button_bg");
}

inline ImVec4 ChatTheme::getButtonHover() {
    return getColor("chat.button_hover");
}

inline ImVec4 ChatTheme::getButtonActive() {
    return getColor("chat.button_active");
}

inline ImVec4 ChatTheme::getButtonIcon() {
    return getColor("chat.button_icon");
}

inline ImVec4 ChatTheme::getButtonIconHover() {
    return getColor("chat.button_icon_hover");
}

inline ImVec4 ChatTheme::getColor(const char* key) {
    return ::DearTs::Core::UI::ThemeManager::instance().getColor(key);
}

} // namespace DearTs::Plugins::Chat::UI
