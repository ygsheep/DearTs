/**
 * @file chat_plugin.cpp
 * @brief Chat 插件实现
 */

#include "chat/chat_plugin.hpp"
#include "chat/views/conversation_list_view.hpp"
#include "chat/views/chat_view.hpp"
#include "chat/views/info_panel_view.hpp"
#include "chat/ui/markdown_renderer.hpp"
#include "chat/llm/llm_interface.hpp"
#include "chat/events/chat_events.hpp"
#include "liblogger/logger.h"
#include <memory>

namespace DearTs::Plugins::Chat {

DearTs::Core::Result<void, std::string> ChatPlugin::on_load() {
    LOG_INFO("Loading Chat plugin...");

    // 初始化 Markdown 渲染器
    UI::MarkdownRendererConfig md_config;
    md_config.link_callback = [](const std::string& url) {
        LOG_INFO("Markdown link clicked: {}", url);
        // TODO: 在系统默认浏览器中打开链接
    };
    UI::MarkdownRenderer::initialize(md_config);
    LOG_INFO("MarkdownRenderer initialized");

    // 创建管理器
    m_conversation_manager = std::make_shared<ConversationManager>();

    // 设置默认 LLM 提供商（Ollama）
    auto ollama_provider = LLM::LLMProviderFactory::create_ollama_provider(
        "http://localhost:11434",  // Ollama 默认地址
        "llama3.2"                  // 默认模型
    );
    LLM::LLMManager::instance().set_provider(std::move(ollama_provider));

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
    return DearTs::Core::Result<void, std::string>::ok();
}

void ChatPlugin::on_unload() {
    LOG_INFO("Unloading Chat plugin...");

    // 清理 Markdown 渲染器
    UI::MarkdownRenderer::cleanup();

    // 清理事件监听
    cleanup_event_listeners();

    // 清理管理器
    m_conversation_manager.reset();
    // LLMManager 是单例，不需要手动清理

    LOG_INFO("Chat plugin unloaded");
}

void ChatPlugin::on_enable() {
    LOG_INFO("Chat plugin enabled");
}

void ChatPlugin::on_disable() {
    LOG_INFO("Chat plugin disabled");
}

void ChatPlugin::register_views() {
    // 注册到 ContentRegistry（使用模板语法）
    using namespace DearTs::Core;
    ContentRegistry::Views::add<ConversationListView>(m_conversation_manager);
    ContentRegistry::Views::add<ChatView>(m_conversation_manager);
    ContentRegistry::Views::add<InfoPanelView>(m_conversation_manager);

    // 设置视图默认打开状态
    auto* conv_list = ContentRegistry::Views::get_by_name("会话");
    if (conv_list) conv_list->get_window_open_state() = true;

    auto* chat = ContentRegistry::Views::get_by_name("聊天");
    if (chat) chat->get_window_open_state() = true;

    auto* info = ContentRegistry::Views::get_by_name("信息");
    if (info) info->get_window_open_state() = true;

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
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::MessageSentEvent>(
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
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::AIAnalysisRequestEvent>(
        [this](const Events::AIAnalysisRequestEvent& e) {
            LOG_INFO("AI analysis requested for conversation {}", e.conversation_id);

            // 发布分析开始事件
            DearTs::Core::Event::EventBus::instance().publish(Events::AIAnalysisStartedEvent{
                .conversation_id = e.conversation_id,
                .task_id = ""
            });

            // 使用 LLM 生成响应
            auto* provider = LLM::LLMManager::instance().get_provider();
            if (!provider) {
                DearTs::Core::Event::EventBus::instance().publish(Events::AIAnalysisCompletedEvent{
                    .conversation_id = e.conversation_id,
                    .task_id = "",
                    .success = false,
                    .error_message = "No LLM provider configured"
                });
                return;
            }

            // 获取会话（需要在流式回调中使用）
            auto conv = m_conversation_manager->find_by_id(e.conversation_id);
            if (!conv) {
                LOG_ERROR("Conversation {} not found", e.conversation_id);
                return;
            }

            // 创建一个空的 AI 消息用于流式输出
            Message ai_msg("", MessageRole::Assistant);
            ai_msg.status = MessageStatus::Sending;
            ai_msg.is_streaming = true;
            ai_msg.displayed_chars = 0;
            conv->add_message(ai_msg);
            conv->touch();

            // 发布消息接收事件（通知 UI 新消息已创建）
            DearTs::Core::Event::EventBus::instance().publish(Events::MessageReceivedEvent{
                .conversation_id = e.conversation_id,
                .message = ai_msg
            });

            // 构建请求
            LLM::LLMRequest request;
            request.prompt = e.current_message.content.empty() ? "你好" : e.current_message.content;
            request.stream = true;  // 启用流式输出

            // 添加上下文
            for (const auto& msg : conv->messages) {
                request.context.push_back(msg.content);
            }

            // 流式输出回调
            request.on_chunk = [this, conversation_id = e.conversation_id](const std::string& chunk) {
                auto conv_ptr = m_conversation_manager->find_by_id(conversation_id);
                if (conv_ptr && !conv_ptr->messages.empty()) {
                    // 获取最后一条消息（AI 消息）
                    auto& msg = conv_ptr->messages.back();
                    if (msg.is_assistant()) {
                        // 追加内容
                        msg.content += chunk;
                        msg.displayed_chars = msg.content.length();
                        msg.is_streaming = true;
                        // 不需要发布事件，UI 会自动检测 is_streaming 状态
                    }
                }
            };

            // 异步发送
            LLM::LLMManager::instance().send_async(request,
                [this, conversation_id = e.conversation_id](const LLM::LLMResponse& response) {
                    auto conv = m_conversation_manager->find_by_id(conversation_id);
                    if (!conv) {
                        LOG_ERROR("Conversation {} not found in response callback", conversation_id);
                        return;
                    }

                    if (response.is_success()) {
                        // 更新最后一条消息的状态
                        if (!conv->messages.empty()) {
                            auto& msg = conv->messages.back();
                            if (msg.is_assistant()) {
                                msg.status = MessageStatus::Sent;
                                msg.is_streaming = false;
                                msg.displayed_chars = msg.content.length();
                            }
                        }
                        conv->touch();

                        // 发布分析完成事件
                        DearTs::Core::Event::EventBus::instance().publish(Events::AIAnalysisCompletedEvent{
                            .conversation_id = conversation_id,
                            .task_id = "",
                            .success = true,
                            .error_message = ""
                        });

                        LOG_INFO("AI response generated for conversation {}", conversation_id);
                    } else {
                        LOG_ERROR("AI analysis failed for conversation {}: {}",
                                  conversation_id, response.error);

                        // 失败时更新消息状态
                        if (!conv->messages.empty()) {
                            auto& msg = conv->messages.back();
                            if (msg.is_assistant()) {
                                msg.status = MessageStatus::Failed;
                                msg.is_streaming = false;
                            }
                        }

                        DearTs::Core::Event::EventBus::instance().publish(Events::AIAnalysisCompletedEvent{
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
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::ConversationCreatedEvent>(
        [this](const Events::ConversationCreatedEvent& e) {
            LOG_INFO("Conversation created: {}", e.conversation->id);
        }
    ));

    // 订阅会话选中事件
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::ConversationSelectedEvent>(
        [this](const Events::ConversationSelectedEvent& e) {
            LOG_INFO("Conversation selected: {}", e.conversation->id);

            // 发布滚动到底部事件
            DearTs::Core::Event::EventBus::instance().publish(Events::ScrollToBottomEvent{
                .conversation_id = e.conversation->id
            });
        }
    ));

    // 订阅滚动到底部事件
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::ScrollToBottomEvent>(
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
