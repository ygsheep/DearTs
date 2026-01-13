/**
 * @file chat_input.hpp
 * @brief 聊天输入框组件
 */

#pragma once

#include <string>
#include <functional>
#include <imgui.h>

namespace DearTs::Plugins::Chat::UI {

/**
 * @brief 聊天输入框样式配置
 */
struct ChatInputStyle {
    ImVec4 bg_color = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
    ImVec4 border_color = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
    ImVec4 focused_border_color = ImVec4(0.3f, 0.5f, 0.8f, 1.0f);
    ImVec4 text_color = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    ImVec4 placeholder_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

    float corner_radius = 8.0f;
    float border_thickness = 1.0f;

    // 最小和最大高度
    float min_height = 40.0f;
    float max_height = 200.0f;

    // 是否显示发送按钮
    bool show_send_button = true;

    // 是否显示 AI 分析按钮
    bool show_ai_button = true;

    // Shift+Enter 换行，Enter 发送
    bool enter_sends = true;
};

/**
 * @brief 聊天输入框组件
 * @details 多行文本输入框，支持发送和 AI 分析
 */
class ChatInput {
public:
    /**
     * @brief 发送回调类型
     */
    using SendCallback = std::function<void(const std::string&)>;

    /**
     * @brief AI 分析回调类型
     */
    using AICallback = std::function<void()>;

    /**
     * @brief 构造函数
     */
    ChatInput();

    /**
     * @brief 绘制输入框
     * @param placeholder 占位符文本
     * @param style 样式配置（可选）
     * @return 用户是否按下发送（true 表示应该发送消息）
     */
    bool draw(
        const char* placeholder = "输入消息...",
        const ChatInputStyle* style = nullptr
    );

    /**
     * @brief 获取输入内容
     */
    [[nodiscard]] const std::string& get_text() const { return m_text; }

    /**
     * @brief 设置输入内容
     */
    void set_text(const std::string& text) {
        m_text = text;
        update_buffer();
    }

    /**
     * @brief 清空输入框
     */
    void clear() {
        m_text.clear();
        m_buffer[0] = '\0';
    }

    /**
     * @brief 设置焦点
     */
    void focus() { m_should_focus = true; }

    /**
     * @brief 是否有焦点
     */
    [[nodiscard]] bool has_focus() const { return m_has_focus; }

    /**
     * @brief 设置发送回调
     */
    void set_send_callback(SendCallback callback) {
        m_send_callback = std::move(callback);
    }

    /**
     * @brief 设置 AI 分析回调
     */
    void set_ai_callback(AICallback callback) {
        m_ai_callback = std::move(callback);
    }

private:
    /**
     * @brief 更新缓冲区
     */
    void update_buffer();

    /**
     * @brief 处理键盘输入
     */
    bool handle_keyboard();

    /**
     * @brief 绘制发送按钮
     */
    bool draw_send_button();

    /**
     * @brief 绘制 AI 按钮
     */
    bool draw_ai_button();

    // 成员变量
    std::string m_text;
    char m_buffer[4096] = "";
    bool m_should_focus = false;
    bool m_has_focus = false;
    bool m_ctrl_enter = false;

    // 回调
    SendCallback m_send_callback;
    AICallback m_ai_callback;
};

} // namespace DearTs::Plugins::Chat::UI
