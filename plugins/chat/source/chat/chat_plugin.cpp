/**
 * @file chat_plugin.cpp
 * @brief Chat 插件实现
 */

#include "chat/chat_plugin.hpp"
#include "chat/views/conversation_list_view.hpp"
#include "chat/views/chat_view.hpp"
#include "chat/views/info_panel_view.hpp"
#include "chat/llm/llm_interface.hpp"
#include "chat/events/chat_events.hpp"
#include "liblogger/logger.h"
#include <memory>

namespace DearTs::Plugins::Chat {

Result<void, std::string> ChatPlugin::on_load() {
    LOG_INFO("Loading Chat plugin...");

    // 创建管理器
    m_conversation_manager = std::make_unique<ConversationManager>();
    m_llm_manager = std::make_unique<LLM::LLMManager>();

    // 设置默认 LLM 提供商（HTTP）
    auto http_provider = LLM::LLMProviderFactory::create_http_provider(
        "http://localhost:11434/v1",  // Ollama 默认地址
        "",                              // API 密钥（Ollama 不需要）
        "llama3.2"                       // 默认模型
    );
    m_llm_manager->set_provider(std::move(http_provider));

    // 注册视图
    register_views();

    // 注册命令
    register_commands();

    // 设置事件监听
    setup_event_listeners();

    // 创建默认会话（如果没有）
    if (m_conversation_manager->get_conversations().empty()) {
        auto default_conv = m_conversation_manager->create_conversation("新对话", ConversationType::AI);
        if (default_conv) {
            // 添加欢迎消息
            Message welcome_msg(
                "你好！我是 AI 助手。有什么可以帮助你的吗？",
                MessageRole::Assistant
            );
            welcome_msg.status = MessageStatus::Sent;
            default_conv->add_message(welcome_msg);
        }
    }

    LOG_INFO("Chat plugin loaded successfully");
    return Result<void, std::string>::ok();
}

void ChatPlugin::on_unload() {
    LOG_INFO("Unloading Chat plugin...");

    // 清理事件监听
    cleanup_event_listeners();

    // 清理管理器
    m_conversation_manager.reset();
    m_llm_manager.reset();

    LOG_INFO("Chat plugin unloaded");
}

void ChatPlugin::on_enable() {
    LOG_INFO("Chat plugin enabled");
}

void ChatPlugin::on_disable() {
    LOG_INFO("Chat plugin disabled");
}

void ChatPlugin::register_views() {
    // 创建三个视图
    auto conv_list_view = std::make_unique<ConversationListView>(m_conversation_manager);
    auto chat_view = std::make_unique<ChatView>(m_conversation_manager);
    auto info_panel_view = std::make_unique<InfoPanelView>(m_conversation_manager);

    // 注册到 ContentRegistry
    // 注意：这里假设 DearTs Framework 已有 ContentRegistry::Views::add
    // 如果没有，需要手动管理视图生命周期
    /*
    ContentRegistry::Views::add(std::move(conv_list_view));
    ContentRegistry::Views::add(std::move(chat_view));
    ContentRegistry::Views::add(std::move(info_panel_view));
    */

    LOG_INFO("Registered Chat views");
}

void ChatPlugin::register_commands() {
    // 注册命令（如果框架支持）
    /*
    ContentRegistry::Commands::register_handler(
        "chat.new_conversation",
        "新建会话",
        [this]() {
            auto conv = m_conversation_manager->create_conversation("新对话", ConversationType::AI);
            if (conv) {
                m_conversation_manager->set_current_conversation(conv);
            }
        },
        nullptr,
        "Ctrl+N"
    );

    ContentRegistry::Commands::register_handler(
        "chat.clear_conversation",
        "清空当前会话",
        [this]() {
            auto conv = m_conversation_manager->get_current_conversation();
            if (conv) {
                conv->clear_messages();
            }
        },
        nullptr,
        "Ctrl+Shift+D"
    );
    */

    LOG_INFO("Registered Chat commands");
}

void ChatPlugin::setup_event_listeners() {
    // 订阅消息发送事件
    m_event_tokens.push_back(EventBus::instance().subscribe<Events::MessageSentEvent>(
        [this](const Events::MessageSentEvent& e) {
            LOG_INFO("Message sent in conversation {}: {}", e.conversation_id, e.message.content);

            // 更新消息状态为已发送
            auto conv = m_conversation_manager->find_by_id(e.conversation_id);
            if (conv && !conv->messages.empty()) {
                conv->messages.back().status = MessageStatus::Sent;
            }
        }
    ));

    // 订阅 AI 分析请求事件
    m_event_tokens.push_back(EventBus::instance().subscribe<Events::AIAnalysisRequestEvent>(
        [this](const Events::AIAnalysisRequestEvent& e) {
            LOG_INFO("AI analysis requested for conversation {}", e.conversation_id);

            // 发布分析开始事件
            EventBus::instance().publish(Events::AIAnalysisStartedEvent{
                .conversation_id = e.conversation_id,
                .task_id = ""
            });

            // 使用 LLM 生成响应
            auto* provider = m_llm_manager->get_provider();
            if (!provider) {
                EventBus::instance().publish(Events::AIAnalysisCompletedEvent{
                    .conversation_id = e.conversation_id,
                    .task_id = "",
                    .success = false,
                    .error_message = "No LLM provider configured"
                });
                return;
            }

            // 构建请求
            LLM::LLMRequest request;
            request.prompt = e.current_message.content.empty() ? "你好" : e.current_message.content;

            // 添加上下文
            auto conv = m_conversation_manager->find_by_id(e.conversation_id);
            if (conv) {
                for (const auto& msg : conv->messages) {
                    request.context.push_back(msg.content);
                }
            }

            // 异步发送
            m_llm_manager->send_async(request,
                [this, conversation_id = e.conversation_id](const LLM::LLMResponse& response) {
                    if (response.is_success()) {
                        // 创建 AI 消息
                        auto conv = m_conversation_manager->find_by_id(conversation_id);
                        if (conv) {
                            Message ai_msg(response.content, MessageRole::Assistant);
                            ai_msg.status = MessageStatus::Sent;
                            conv->add_message(ai_msg);
                            conv->touch();

                            // 发布消息接收事件
                            EventBus::instance().publish(Events::MessageReceivedEvent{
                                .conversation_id = conversation_id,
                                .message = ai_msg
                            });

                            // 发布分析完成事件
                            EventBus::instance().publish(Events::AIAnalysisCompletedEvent{
                                .conversation_id = conversation_id,
                                .task_id = "",
                                .success = true,
                                .error_message = ""
                            });

                            LOG_INFO("AI response generated for conversation {}", conversation_id);
                        }
                    } else {
                        LOG_ERROR("AI analysis failed for conversation {}: {}",
                                  conversation_id, response.error);

                        EventBus::instance().publish(Events::AIAnalysisCompletedEvent{
                            .conversation_id = conversation_id,
                            .task_id = "",
                            .success = false,
                            .error_message = response.error
                        });
                    }
                }
            );
        }
    ));

    // 订阅会话创建事件
    m_event_tokens.push_back(EventBus::instance().subscribe<Events::ConversationCreatedEvent>(
        [this](const Events::ConversationCreatedEvent& e) {
            LOG_INFO("Conversation created: {}", e.conversation->id);
        }
    ));

    // 订阅会话选中事件
    m_event_tokens.push_back(EventBus::instance().subscribe<Events::ConversationSelectedEvent>(
        [this](const Events::ConversationSelectedEvent& e) {
            LOG_INFO("Conversation selected: {}", e.conversation->id);

            // 发布滚动到底部事件
            EventBus::instance().publish(Events::ScrollToBottomEvent{
                .conversation_id = e.conversation->id
            });
        }
    ));

    // 订阅滚动到底部事件
    m_event_tokens.push_back(EventBus::instance().subscribe<Events::ScrollToBottomEvent>(
        [this](const Events::ScrollToBottomEvent& e) {
            // 这个事件会由 ChatView 处理
        }
    ));

    LOG_INFO("Set up Chat event listeners");
}

void ChatPlugin::cleanup_event_listeners() {
    // EventToken 的 RAII 会自动取消订阅
    m_event_tokens.clear();
    LOG_INFO("Cleaned up Chat event listeners");
}

} // namespace DearTs::Plugins::Chat
