/**
 * @file info_panel_view.hpp
 * @brief 信息面板视图（右侧）
 */

#pragma once

#include "core/ui/view.h"
#include "core/ui/icon_font.hpp"
#include "core/event/event_bus.h"
#include "chat/models/conversation.hpp"
#include <memory>

namespace DearTs::Plugins::Chat {

using Core::ContentRegistry::UnlocalizedString;
using Core::UI::ViewWindow;

/**
 * @brief 信息面板视图
 * @details 显示 AI 设置、会话信息、导出选项等
 */
class InfoPanelView : public ViewWindow {
public:
    explicit InfoPanelView(std::shared_ptr<ConversationManager> manager);
    ~InfoPanelView() override;

    void draw_content() override;
    ImVec2 get_min_size() const override { return ImVec2(300, 400); }

private:
    /**
     * @brief 设置事件监听器
     */
    void setup_event_listeners();
    /**
     * @brief 绘制 AI 设置部分
     */
    void draw_ai_settings();

    /**
     * @brief 绘制 LLM 提供商选择器
     */
    void draw_llm_provider_selector();

    /**
     * @brief 绘制 Ollama 设置
     */
    void draw_ollama_settings();

    /**
     * @brief 刷新 Ollama 模型列表
     */
    void refresh_ollama_models();

    /**
     * @brief 设置可用模型列表
     */
    void set_available_models(const std::vector<std::string>& models);

    /**
     * @brief 设置 Ollama 连接状态
     */
    void set_ollama_connection_status(bool connected, const std::string& error = "");

    /**
     * @brief 绘制模型设置
     */
    void draw_model_settings();

    /**
     * @brief 绘制会话信息部分
     */
    void draw_conversation_info();

    /**
     * @brief 绘制导出部分
     */
    void draw_export_section();

    /**
     * @brief 绘制调试部分（缓存统计）
     */
    void draw_debug_section();

    /**
     * @brief 绘制测试消息部分
     */
    void draw_test_messages_section();

    /**
     * @brief 添加测试消息
     */
    void add_test_message(const std::string& content, MessageRole role);

    /**
     * @brief 切换 LLM 提供商
     */
    void change_llm_provider(const std::string& provider);

    /**
     * @brief 切换模型
     */
    void change_model(const std::string& model);

    /**
     * @brief 导出会话
     */
    void export_conversation(const std::string& format);

    // 成员变量
    std::shared_ptr<ConversationManager> m_conversation_manager;

    // LLM 设置
    std::string m_selected_provider = "ollama";
    std::string m_selected_model = "llama3.2";
    std::vector<std::string> m_available_providers = {"ollama", "http", "python", "cli"};
    std::vector<std::string> m_available_models = {"llama3.2"};

    // Ollama 设置
    std::string m_ollama_base_url = "http://localhost:11434";
    bool m_ollama_connected = false;
    std::string m_ollama_connection_error;
    bool m_ollama_refreshing = false;

    // 参数设置
    float m_temperature = 0.7f;
    int m_max_tokens = 2048;

    // 导出格式
    std::string m_export_format = "json";
    std::string m_export_path = "";

    // UI 状态
    bool m_show_advanced = false;

    // 事件订阅 Token（RAII 自动清理）
    std::vector<DearTs::Core::Event::EventToken> m_event_tokens;
};

} // namespace DearTs::Plugins::Chat
