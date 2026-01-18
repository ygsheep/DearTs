/**
 * @file chat_plugin.cpp
 * @brief Chat 插件实现
 */

#include "chat/chat_plugin.hpp"
#include "chat/views/conversation_list_view.hpp"
#include "chat/views/chat_view.hpp"
#include "chat/views/input_view.hpp"
#include "chat/views/info_panel_view.hpp"
#include "chat/ui/markdown_renderer.hpp"
#include "chat/llm/llm_interface.hpp"
#include "chat/llm/ollama_llm_provider.hpp"
#include "chat/events/chat_events.hpp"
#include "memory_core/events/memory_events.hpp"
#include "memory_core/persistence/database.hpp"
#include "core/tasks/task_manager.h"
#include "liblogger/logger.h"
#include <imgui.h>
#include <memory>
#include <thread>
#include <chrono>

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

    // 字体已在 application.cpp 中配置，这里不需要重复设置

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

    // ✅ 不再在这里创建默认会话
    // 默认会话将在后台任务加载历史数据时创建（如果数据库为空）
    // 或者用户可以手动创建新会话

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

    // 检查当前 LLM 提供商是否为 Ollama，如果是则在后台刷新模型列表
    auto* provider = LLM::LLMManager::instance().get_provider();
    if (provider && provider->get_name() == "Ollama") {
        LOG_INFO("Ollama provider detected, launching background model list refresh");

        // 使用任务系统在后台刷新模型列表
        DearTs::Core::Tasks::TaskManager::instance().launch(
            "Refresh Ollama Models",
            [this](const std::atomic<bool>& should_cancel) {
                // 稍微延迟一下，避免在启动时阻塞
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                if (should_cancel) {
                    LOG_INFO("Ollama model refresh cancelled");
                    return;
                }

                // 执行刷新
                auto* provider = LLM::LLMManager::instance().get_provider();
                if (!provider) {
                    LOG_ERROR("No LLM provider set during background refresh");
                    return;
                }

                // 检查是否是 Ollama 提供商
                if (provider->get_name() != "Ollama") {
                    LOG_WARN("Provider changed during background refresh, not Ollama: {}", provider->get_name());
                    return;
                }

                // 尝试转换为 OllamaLLMProvider
                auto* ollama_provider = dynamic_cast<LLM::OllamaLLMProvider*>(provider);
                if (!ollama_provider) {
                    LOG_ERROR("Failed to cast provider to OllamaLLMProvider");
                    return;
                }

                // 获取模型列表
                try {
                    std::vector<std::string> models = ollama_provider->get_models();
                    LOG_INFO("Background refresh retrieved {} models from Ollama", models.size());

                    // 发布模型列表更新事件（使用默认 URL）
                    DearTs::Core::Event::EventBus::instance().publish(Events::OllamaModelsUpdatedEvent{
                        .models = models,
                        .base_url = "http://localhost:11434"
                    });

                    // 发布连接状态事件（成功）
                    DearTs::Core::Event::EventBus::instance().publish(Events::OllamaConnectionStatusEvent{
                        .is_connected = true,
                        .base_url = "http://localhost:11434",
                        .error_message = ""
                    });

                } catch (const std::exception& e) {
                    LOG_ERROR("Background Ollama model refresh failed: {}", e.what());

                    // 发布连接状态事件（失败）
                    DearTs::Core::Event::EventBus::instance().publish(Events::OllamaConnectionStatusEvent{
                        .is_connected = false,
                        .base_url = "http://localhost:11434",
                        .error_message = e.what()
                    });
                }
            },
            DearTs::Core::Tasks::TaskType::Background  // 后台任务，不影响 UI
        );

        LOG_INFO("Launched background task to refresh Ollama models");
    }

    // ✅ 启动后台任务：加载最近 30 天的历史会话
    LOG_INFO("Launching background task to load conversation history");

    DearTs::Core::Tasks::TaskManager::instance().launch(
        "Load Conversation History",
        [this](const std::atomic<bool>& should_cancel) {
            // 稍微延迟一下，避免在启动时阻塞
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            if (should_cancel) {
                LOG_INFO("Conversation history loading cancelled");
                return;
            }

            // 计算时间范围：最近 30 天
            // ✅ 修复时间戳计算问题
            auto now = std::chrono::system_clock::now();
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count();

            // 30 天 = 30 * 24 * 60 * 60 * 1000 = 2,592,000,000 毫秒
            constexpr int64_t thirty_days_ms = 30 * 24 * 60 * 60 * 1000;

            // ✅ DEBUG: 输出当前时间的日期
            auto now_days = now_ms / 86400000;
            auto now_years = 1970 + (now_days / 365);
            LOG_INFO("DEBUG: Current timestamp {} ms = {} days since epoch = year {}",
                     now_ms, now_days, now_years);

            auto thirty_days_ago_ms = now_ms - thirty_days_ms;

            LOG_INFO("Current time: {} ms (approximately {} days since epoch)", now_ms, now_ms / 86400000);
            LOG_INFO("Loading conversations from {} to {} (last 30 days)",
                     thirty_days_ago_ms, now_ms);

            // ✅ 由于系统时钟可能有问题，直接加载所有会话
            // 这样可以确保所有历史会话都能被加载
            auto& db = DearTs::Plugins::MemoryCore::Persistence::SQLiteDatabase::instance();
            LOG_INFO("Loading all conversations from database (ignoring time range due to potential clock issues)");

            auto result = db.get_all_conversations();

            if (result.isErr()) {
                LOG_ERROR("Failed to load conversations from database: {}", result.error());
                return;
            }

            auto records = result.unwrap();
            LOG_INFO("Found {} conversations in database", records.size());

            // ✅ 如果数据库为空，创建默认会话
            if (records.empty()) {
                LOG_INFO("Database is empty, creating default conversation");
                auto default_conv = m_conversation_manager->create_conversation("新对话", ConversationType::AI);
                if (default_conv) {
                    // 添加欢迎消息
                    Message welcome_msg(
                        "你好！我是 AI 助手。有什么可以帮助你的吗？",
                        MessageRole::Assistant
                    );
                    welcome_msg.status = MessageStatus::Sent;
                    default_conv->add_message(welcome_msg);
                    LOG_INFO("Created default conversation");
                }
                return;  // 创建完默认会话后返回
            }

            // 加载每个会话到 ConversationManager（消息懒加载，稍后按需加载）
            int loaded_count = 0;
            for (const auto& record : records) {
                if (should_cancel) {
                    LOG_INFO("Conversation history loading cancelled after {} items", loaded_count);
                    return;
                }

                // 转换类型字符串到 ConversationType
                ConversationType type = ConversationType::AI;
                if (record.type == "chat") {
                    type = ConversationType::AI;
                }

                // 加载会话（不加载消息，懒加载）
                m_conversation_manager->load_conversation(
                    record.id,
                    record.title,
                    type,
                    record.created_at,
                    record.updated_at
                );

                loaded_count++;
            }

            LOG_INFO("Loaded {} conversations from database (last 30 days)", loaded_count);
        },
        DearTs::Core::Tasks::TaskType::Background  // 后台任务，静默加载
    );
}

void ChatPlugin::on_disable() {
    LOG_INFO("Chat plugin disabled");
}

void ChatPlugin::register_views() {
    // 注册到 ContentRegistry（使用模板语法）
    using namespace DearTs::Core;
    ContentRegistry::Views::add<ConversationListView>(m_conversation_manager);
    ContentRegistry::Views::add<ChatView>(m_conversation_manager);
    ContentRegistry::Views::add<InputView>(m_conversation_manager);
    ContentRegistry::Views::add<InfoPanelView>(m_conversation_manager);

    // 设置视图默认打开状态
    auto* conv_list = ContentRegistry::Views::get_by_name("会话");
    if (conv_list) conv_list->get_window_open_state() = true;

    auto* chat = ContentRegistry::Views::get_by_name("聊天");
    if (chat) chat->get_window_open_state() = true;

    auto* input = ContentRegistry::Views::get_by_name("输入");
    if (input) input->get_window_open_state() = true;

    auto* info = ContentRegistry::Views::get_by_name("信息");
    if (info) info->get_window_open_state() = true;

    // 保存 InfoPanelView 指针用于后续更新
    m_info_panel = dynamic_cast<InfoPanelView*>(info);

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

            // ✅ 发布 memory_core 事件：请求保存消息到数据库
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count();

            // 转换 MessageRole 枚举为字符串
            std::string role_str;
            switch (e.message.role) {
                case MessageRole::User: role_str = "user"; break;
                case MessageRole::Assistant: role_str = "assistant"; break;
                case MessageRole::System: role_str = "system"; break;
                default: role_str = "user"; break;
            }

            MemoryCore::Events::MessageSaveRequestedEvent save_event;
            save_event.conversation_id = e.conversation_id;
            save_event.message_uuid = e.message.id;  // 使用 id 而不是 uuid
            save_event.role = role_str;
            save_event.content = e.message.content;
            save_event.timestamp = timestamp;
            save_event.tokens = e.message.token_count;
            save_event.conversation_title = conv ? conv->title : "新对话";

            DearTs::Core::Event::EventBus::instance().publish(save_event);

            // 发布 memory_core 事件：请求记忆提取
            std::vector<std::string> message_contents;
            message_contents.push_back(e.message.content);

            DearTs::Plugins::MemoryCore::Events::MemoryExtractRequestedEvent extract_event;
            extract_event.conversation_id = e.conversation_id;
            extract_event.message_contents = message_contents;
            extract_event.use_llm = true;  // 使用 LLM 提取

            DearTs::Core::Event::EventBus::instance().publish(extract_event);
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
            LOG_INFO("Conversation created: id={}, title={}", e.conversation_id, e.title);
        }
    ));

    // 订阅会话选中事件
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::ConversationSelectedEvent>(
        [this](const Events::ConversationSelectedEvent& e) {
            LOG_INFO("Conversation selected: {}", e.conversation->id);
            LOG_DEBUG("Conversation has {} messages in memory", e.conversation->messages.size());

            // ✅ 总是从数据库加载消息（确保显示最新的历史记录）
            // 如果内存中已有消息，load_messages 会跳过（见 conversation.cpp:260-264）
            LOG_INFO("Loading messages for conversation {} from database", e.conversation->id);
            auto result = m_conversation_manager->load_messages(e.conversation->id);
            if (result.isErr()) {
                LOG_ERROR("Failed to load messages for conversation {}: {}",
                         e.conversation->id, result.error());
            } else {
                auto count = result.unwrap();
                LOG_INFO("Loaded {} messages for conversation {} (now has {} in memory)",
                         count, e.conversation->id, e.conversation->messages.size());
            }

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

    // 订阅 Ollama 模型列表更新事件
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::OllamaModelsUpdatedEvent>(
        [this](const Events::OllamaModelsUpdatedEvent& e) {
            if (e.models.empty()) {
                // 空列表表示请求刷新，执行实际的刷新
                handle_ollama_models_refresh(e.base_url);
            }
            // ✅ 注意：不再在处理器中发布新的事件，避免无限循环
            // InfoPanelView 会直接监听这个事件并更新 UI
        }
    ));

    // ========== memory_core 事件订阅 ==========

    // 订阅记忆提取完成事件
    m_event_tokens.push_back(
        DearTs::Core::Event::EventBus::instance().subscribe<DearTs::Plugins::MemoryCore::Events::MemoryExtractedEvent>(
            [this](const DearTs::Plugins::MemoryCore::Events::MemoryExtractedEvent& e) {
                if (e.success) {
                    LOG_INFO("Memory extracted: {} memories for conversation {}",
                             e.memories.size(), e.conversation_id);
                } else {
                    LOG_WARN("Memory extraction failed for conversation {}: {}",
                              e.conversation_id, e.error_message);
                }

                // TODO: 可以在这里添加 UI 反馈，例如在 InfoPanelView 中显示提取的记忆数量
            }
        )
    );

    // 订阅消息保存完成事件
    m_event_tokens.push_back(
        DearTs::Core::Event::EventBus::instance().subscribe<DearTs::Plugins::MemoryCore::Events::MessageSavedEvent>(
            [this](const DearTs::Plugins::MemoryCore::Events::MessageSavedEvent& e) {
                if (e.success) {
                    LOG_INFO("Message saved: UUID={}, DB_ID={} for conversation {}",
                             e.message_uuid, e.database_id, e.conversation_id);
                } else {
                    LOG_ERROR("Failed to save message for conversation {}: {}",
                               e.conversation_id, e.error_message);
                }
            }
        )
    );

    // 订阅 RAG 查询完成事件
    m_event_tokens.push_back(
        DearTs::Core::Event::EventBus::instance().subscribe<DearTs::Plugins::MemoryCore::Events::RAGQueryCompletedEvent>(
            [this](const DearTs::Plugins::MemoryCore::Events::RAGQueryCompletedEvent& e) {
                if (e.success) {
                    LOG_INFO("RAG query completed: {} results for query '{}'",
                             e.results.size(), e.query);
                } else {
                    LOG_WARN("RAG query failed: {}", e.error_message);
                }

                // TODO: 可以将检索到的记忆显示在 InfoPanelView 中
            }
        )
    );

    LOG_INFO("Set up Chat event listeners (including memory_core integration)");
}

void ChatPlugin::cleanup_event_listeners() {
    // EventToken 的 RAII 会自动取消订阅
    m_event_tokens.clear();
    LOG_INFO("Cleaned up Chat event listeners");
}

void ChatPlugin::handle_ollama_models_refresh(const std::string& base_url) {
    LOG_INFO("Refreshing Ollama models from {}", base_url);

    // 获取当前 LLM 提供商
    auto* provider = LLM::LLMManager::instance().get_provider();
    if (!provider) {
        LOG_ERROR("No LLM provider set");
        return;
    }

    // 检查是否是 Ollama 提供商
    if (provider->get_name() != "Ollama") {
        LOG_WARN("Current provider is not Ollama: {}", provider->get_name());
        return;
    }

    // 尝试转换为 OllamaLLMProvider
    auto* ollama_provider = dynamic_cast<LLM::OllamaLLMProvider*>(provider);
    if (!ollama_provider) {
        LOG_ERROR("Failed to cast provider to OllamaLLMProvider");
        return;
    }

    // 获取模型列表
    try {
        std::vector<std::string> models = ollama_provider->get_models();
        LOG_INFO("Retrieved {} models from Ollama", models.size());

        // ✅ 只有在获取到模型列表时才发布更新事件
        // 如果列表为空（Ollama 未运行或无模型），不发布事件以避免无限循环
        if (!models.empty()) {
            // 发布模型列表更新事件
            DearTs::Core::Event::EventBus::instance().publish(Events::OllamaModelsUpdatedEvent{
                .models = models,
                .base_url = base_url
            });

            // 发布连接状态事件（成功）
            DearTs::Core::Event::EventBus::instance().publish(Events::OllamaConnectionStatusEvent{
                .is_connected = true,
                .base_url = base_url,
                .error_message = ""
            });
        } else {
            // 模型列表为空，只发布连接状态但标记为未连接
            LOG_WARN("Ollama returned empty model list, might not be available");
            DearTs::Core::Event::EventBus::instance().publish(Events::OllamaConnectionStatusEvent{
                .is_connected = false,
                .base_url = base_url,
                .error_message = "No models available"
            });
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get Ollama models: {}", e.what());

        // 发布连接状态事件（失败）
        DearTs::Core::Event::EventBus::instance().publish(Events::OllamaConnectionStatusEvent{
            .is_connected = false,
            .base_url = base_url,
            .error_message = e.what()
        });
    }
}

void ChatPlugin::handle_ollama_connection_test(const std::string& base_url) {
    LOG_INFO("Testing Ollama connection to {}", base_url);

    // 获取当前 LLM 提供商
    auto* provider = LLM::LLMManager::instance().get_provider();
    if (!provider) {
        LOG_ERROR("No LLM provider set");
        return;
    }

    // 检查是否是 Ollama 提供商
    if (provider->get_name() != "Ollama") {
        LOG_WARN("Current provider is not Ollama: {}", provider->get_name());

        // 发布失败状态
        DearTs::Core::Event::EventBus::instance().publish(Events::OllamaConnectionStatusEvent{
            .is_connected = false,
            .base_url = base_url,
            .error_message = "Provider is not Ollama"
        });
        return;
    }

    // 尝试转换为 OllamaLLMProvider
    auto* ollama_provider = dynamic_cast<LLM::OllamaLLMProvider*>(provider);
    if (!ollama_provider) {
        LOG_ERROR("Failed to cast provider to OllamaLLMProvider");

        DearTs::Core::Event::EventBus::instance().publish(Events::OllamaConnectionStatusEvent{
            .is_connected = false,
            .base_url = base_url,
            .error_message = "Failed to access Ollama provider"
        });
        return;
    }

    // 测试连接
    bool connected = ollama_provider->is_available();
    LOG_INFO("Ollama connection test result: {}", connected ? "Connected" : "Failed");

    // 发布连接状态事件
    DearTs::Core::Event::EventBus::instance().publish(Events::OllamaConnectionStatusEvent{
        .is_connected = connected,
        .base_url = base_url,
        .error_message = connected ? "" : "Connection failed"
    });
}

} // namespace DearTs::Plugins::Chat
