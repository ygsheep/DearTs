/**
 * @file chat_plugin.hpp
 * @brief Chat 插件入口
 */

#pragma once

#include "core/plugin/plugin.h"
#include "core/event/event_bus.h"
#include "chat/models/conversation.hpp"
#include "chat/llm/llm_interface.hpp"
#include <memory>

namespace DearTs::Plugins::Chat {

/**
 * @brief Chat 插件主类
 */
class ChatPlugin : public Core::Plugin::IPlugin {
public:
    ChatPlugin() = default;
    ~ChatPlugin() override = default;

    // IPlugin 接口实现
    [[nodiscard]] Core::Plugin::PluginInfo get_info() const override {
        return Core::Plugin::PluginInfo{
            .name = "Chat",
            .author = "DearTs Team",
            .description = "Modern chat GUI with AI assistance",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    [[nodiscard]] DearTs::Core::Result<void, std::string> on_load() override;

    void on_unload() override;
    void on_enable() override;
    void on_disable() override;

    /**
     * @brief 获取会话管理器
     */
    [[nodiscard]] ConversationManager* get_conversation_manager() const {
        return m_conversation_manager.get();
    }

    /**
     * @brief 获取 LLM 管理器
     */
    [[nodiscard]] LLM::LLMManager* get_llm_manager() const {
        return &LLM::LLMManager::instance();
    }

private:
    /**
     * @brief 注册视图
     */
    void register_views();

    /**
     * @brief 注册命令
     */
    void register_commands();

    /**
     * @brief 设置事件监听
     */
    void setup_event_listeners();

    /**
     * @brief 清理事件监听
     */
    void cleanup_event_listeners();

    // 管理器
    std::shared_ptr<ConversationManager> m_conversation_manager;
    // LLMManager 是单例，使用 instance() 获取

    // 事件订阅 Token（RAII 自动清理）
    std::vector<DearTs::Core::Event::EventToken> m_event_tokens;
};

} // namespace DearTs::Plugins::Chat
