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
#include "chat/llm/http_llm_provider.hpp"
#include "chat/events/chat_events.hpp"
#include "memory_core/events/memory_events.hpp"
#include "memory_core/persistence/database.hpp"
#include "core/tasks/task_manager.h"
#include "core/network/http_client.hpp"
#include "core/network/http_types.hpp"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <memory>
#include <thread>
#include <chrono>
#include <set>
#include <functional>

namespace DearTs::Plugins::Chat {

using json = nlohmann::json;

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

    // 从配置读取 LLM 提供商类型
    auto& config_manager = DearTs::Core::Config::ConfigManager::instance();
    DearTs::Core::Config::ConfigScope config("chat");

    // 读取提供商配置
    std::string provider_id = config.get_or<std::string>("llm.provider", "ollama");
    std::string model = config.get_or<std::string>("llm.model", "llama3.2");
    std::string api_key = config.get_or<std::string>("llm.api_key", "");

    // 根据提供商创建对应的 LLM provider
    std::unique_ptr<LLM::ILLMProvider> provider;

    if (provider_id == "ollama") {
        std::string ollama_base_url = config.get_or<std::string>("llm.ollama_base_url", "http://localhost:11434");
        LOG_INFO("Creating Ollama provider with model: {}", model);
        provider = LLM::LLMProviderFactory::create_ollama_provider(ollama_base_url, model);
    } else if (provider_id == "llmstudio") {
        std::string llm_studio_base_url = config.get_or<std::string>("llm.llm_studio_base_url", "http://localhost:1234/v1");
        LOG_INFO("Creating LLM Studio provider with model: {}", model);
        provider = LLM::LLMProviderFactory::create_llm_studio_provider(llm_studio_base_url, model);
    } else {
        // 云端提供商（OpenAI、DeepSeek、Qwen、Zhipu、Zai）
        std::string base_url = config.get_or<std::string>("llm.custom_base_url", "");
        LOG_INFO("Creating HTTP provider ({}) with model: {}", provider_id, model);
        provider = LLM::LLMProviderFactory::create_http_provider(base_url, api_key, model);
    }

    if (provider) {
        LLM::LLMManager::instance().set_provider(std::move(provider));
    } else {
        LOG_ERROR("Failed to create LLM provider for: {}", provider_id);
    }

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

    // 从配置读取当前提供商
    DearTs::Core::Config::ConfigScope config("chat");
    std::string provider_id = config.get_or<std::string>("llm.provider", "ollama");

    // 检查是否为本地提供商（Ollama 或 LLM Studio），需要刷新模型列表
    if (provider_id == "ollama") {
        LOG_INFO("Ollama provider detected, launching background model list refresh");

        DearTs::Core::Tasks::TaskManager::instance().launch(
            "Refresh Ollama Models",
            [this](const std::atomic<bool>& should_cancel) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if (should_cancel) return;

                auto* provider = LLM::LLMManager::instance().get_provider();
                if (!provider || provider->get_name() != "Ollama") {
                    LOG_WARN("Provider changed during background refresh");
                    return;
                }

                auto* ollama_provider = dynamic_cast<LLM::OllamaLLMProvider*>(provider);
                if (!ollama_provider) {
                    LOG_ERROR("Failed to cast provider to OllamaLLMProvider");
                    return;
                }

                try {
                    std::vector<std::string> models = ollama_provider->get_models();
                    LOG_INFO("Background refresh retrieved {} models from Ollama", models.size());

                    DearTs::Core::Event::EventBus::instance().publish(Events::LLMModelsUpdatedEvent{
                        .models = models,
                        .base_url = "http://localhost:11434",
                        .provider_type = Events::LLMProviderType::Ollama
                    });

                    DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
                        .is_connected = true,
                        .base_url = "http://localhost:11434",
                        .error_message = "",
                        .provider_type = Events::LLMProviderType::Ollama
                    });

                } catch (const std::exception& e) {
                    LOG_ERROR("Background Ollama model refresh failed: {}", e.what());
                    DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
                        .is_connected = false,
                        .base_url = "http://localhost:11434",
                        .error_message = e.what(),
                        .provider_type = Events::LLMProviderType::Ollama
                    });
                }
            },
            DearTs::Core::Tasks::TaskType::Background
        );
        LOG_INFO("Launched background task to refresh Ollama models");

    } else if (provider_id == "llmstudio") {
        LOG_INFO("LLM Studio provider detected, launching background model list refresh");

        DearTs::Core::Tasks::TaskManager::instance().launch(
            "Refresh LLM Studio Models",
            [this](const std::atomic<bool>& should_cancel) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if (should_cancel) return;

                auto* provider = LLM::LLMManager::instance().get_provider();
                if (!provider || provider->get_name() != "HTTP") {
                    LOG_WARN("Provider changed during background refresh");
                    return;
                }

                auto* http_provider = dynamic_cast<LLM::HTTPLLMProvider*>(provider);
                if (!http_provider) {
                    LOG_ERROR("Failed to cast provider to HTTPLLMProvider");
                    return;
                }

                try {
                    std::vector<std::string> models = http_provider->get_models();
                    LOG_INFO("Background refresh retrieved {} models from LLM Studio", models.size());

                    DearTs::Core::Event::EventBus::instance().publish(Events::LLMModelsUpdatedEvent{
                        .models = models,
                        .base_url = "http://localhost:1234/v1",
                        .provider_type = Events::LLMProviderType::LLMStudio
                    });

                    DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
                        .is_connected = true,
                        .base_url = "http://localhost:1234/v1",
                        .error_message = "",
                        .provider_type = Events::LLMProviderType::LLMStudio
                    });

                } catch (const std::exception& e) {
                    LOG_ERROR("Background LLM Studio model refresh failed: {}", e.what());
                    DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
                        .is_connected = false,
                        .base_url = "http://localhost:1234/v1",
                        .error_message = e.what(),
                        .provider_type = Events::LLMProviderType::LLMStudio
                    });
                }
            },
            DearTs::Core::Tasks::TaskType::Background
        );
        LOG_INFO("Launched background task to refresh LLM Studio models");
    } else {
        LOG_INFO("Cloud provider detected ({}), no model refresh needed", provider_id);
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
            constexpr int64_t thirty_days_ms = 30LL * 24 * 60 * 60 * 1000;

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

void ChatPlugin::cancel_pending_refresh_tasks() {
    if (m_pending_refresh_task && !m_pending_refresh_task->isFinished()) {
        LOG_INFO("Cancelling pending refresh task before switching provider");
        m_pending_refresh_task->cancel();
        m_pending_refresh_task.reset();
    }
}

void ChatPlugin::setup_event_listeners() {
    // 订阅消息发送事件
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::MessageSentEvent>(
        [this](const Events::MessageSentEvent& e) {
            LOG_INFO("MessageSentEvent received: conv_id={}, msg_id={}, role={}, content='{}...'",
                     e.conversation_id, e.message.id,
                     static_cast<int>(e.message.role),
                     e.message.content.substr(0, std::min(size_t(30), e.message.content.length())));

            // 更新消息状态为已发送
            auto conv = m_conversation_manager->find_by_id(e.conversation_id);
            if (conv && !conv->messages.empty()) {
                conv->messages.back().status = MessageStatus::Sent;
            } else {
                LOG_WARN("Conversation {} not found or empty when handling MessageSentEvent", e.conversation_id);
            }

            // 捕获需要的数据
            auto conversation_id = e.conversation_id;
            auto message_id = e.message.id;
            auto message_role = e.message.role;
            auto message_content = e.message.content;
            auto message_tokens = e.message.token_count;
            auto conversation_title = conv ? conv->title : "新对话";

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

            // ✅ 使用 publish_async 保存消息到数据库（避免阻塞 UI，但确保及时保存）
            // 注意：publish_async 会在下一帧的 process_async_events 中处理
            MemoryCore::Events::MessageSaveRequestedEvent save_event;
            save_event.conversation_id = conversation_id;
            save_event.message_uuid = message_id;
            save_event.role = role_str;
            save_event.content = message_content;
            save_event.timestamp = timestamp;
            save_event.tokens = message_tokens;
            save_event.conversation_title = conversation_title;

            LOG_INFO("[USER MSG] Publishing MessageSaveRequestedEvent: conv_id={}, msg_uuid={}, role='{}', content_len={}",
                     save_event.conversation_id, save_event.message_uuid,
                     save_event.role, save_event.content.length());

            DearTs::Core::Event::EventBus::instance().publish_async(save_event);

            LOG_INFO("[USER MSG] MessageSaveRequestedEvent published to async queue");

            // 发布 memory_core 事件：请求记忆提取（异步）
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
            // Ollama 在单个 HTTP chunk 内的多个 NDJSON 行是累积的，
            // 但跨 chunk 时内容是增量的，需要追加
            // 使用 shared_ptr 来共享去重集合，避免捕获问题
            auto processed_chunks = std::make_shared<std::set<size_t>>();  // 存储已处理的 chunk 内容哈希

            request.on_chunk = [this, conversation_id = e.conversation_id, processed_chunks](const std::string& chunk) {
                // 计算 chunk 的简单哈希（用于去重）
                size_t chunk_hash = std::hash<std::string>{}(chunk);

                // 检查是否已经处理过这个 chunk
                if (processed_chunks->find(chunk_hash) != processed_chunks->end()) {
                    LOG_DEBUG("UI on_chunk: skipping duplicate chunk (hash={})", chunk_hash);
                    return;  // 跳过重复的 chunk
                }

                // 标记为已处理
                processed_chunks->insert(chunk_hash);

                auto conv_ptr = m_conversation_manager->find_by_id(conversation_id);
                if (conv_ptr && !conv_ptr->messages.empty()) {
                    // 获取最后一条消息（AI 消息）
                    auto& msg = conv_ptr->messages.back();
                    if (msg.is_assistant()) {
                        // 记录追加前的状态
                        const std::string before = msg.content;
                        // 追加新内容（ollama_llm_provider 已处理 chunk 内的累积问题）
                        msg.content += chunk;
                        msg.displayed_chars = msg.content.length();
                        msg.is_streaming = true;

                        // 详细日志：追踪每次追加
                        // LOG_INFO("UI on_chunk: chunk='{}' (len={}), before='{}...', after='{}...', total_len={})",
                        //         chunk.substr(0, std::min(size_t(20), chunk.length())),
                        //         chunk.length(),
                        //         before.substr(0, std::min(size_t(20), before.length())),
                        //         msg.content.substr(0, std::min(size_t(20), msg.content.length())),
                        //         msg.content.length());
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

                                // ✅ 直接保存 AI 消息到数据库（此回调已在后台线程中执行）
                                auto now = std::chrono::system_clock::now();
                                auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    now.time_since_epoch()
                                ).count();

                                MemoryCore::Events::MessageSaveRequestedEvent save_event;
                                save_event.conversation_id = conversation_id;
                                save_event.message_uuid = msg.id;
                                save_event.role = "assistant";
                                save_event.content = msg.content;
                                save_event.timestamp = timestamp;
                                save_event.tokens = msg.token_count;
                                save_event.conversation_title = conv->title;

                                DearTs::Core::Event::EventBus::instance().publish(save_event);

                                LOG_INFO("AI message saved to database: conv_id={}, uuid={}",
                                         conversation_id, msg.id);
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

            // ✅ 先处理所有待处理的异步事件（确保消息已保存）
            // 这样可以避免：发送消息 → 立即切换会话 → 用户消息还没保存的问题
            DearTs::Core::Event::EventBus::instance().process_async_events();

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

    // 订阅 LLM 模型列表更新事件
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::LLMModelsUpdatedEvent>(
        [this](const Events::LLMModelsUpdatedEvent& e) {
            if (e.models.empty()) {
                // 空列表表示请求刷新
                // ✅ 使用 TaskManager 异步处理网络请求，避免阻塞 UI
                if (e.provider_type == Events::LLMProviderType::LLMStudio) {
                    LOG_INFO("Scheduling async LLM Studio model refresh from {}", e.base_url);
                    DearTs::Core::Tasks::TaskManager::instance().launch(
                        "Refresh LLM Studio Models",
                        [this, base_url = e.base_url](const auto&) {
                            handle_llm_studio_models_refresh(base_url);
                        },
                        DearTs::Core::Tasks::TaskType::Background
                    );
                } else if (e.provider_type == Events::LLMProviderType::Ollama) {
                    LOG_INFO("Scheduling async Ollama model refresh from {}", e.base_url);
                    DearTs::Core::Tasks::TaskManager::instance().launch(
                        "Refresh Ollama Models",
                        [this, base_url = e.base_url](const auto&) {
                            handle_ollama_models_refresh(base_url);
                        },
                        DearTs::Core::Tasks::TaskType::Background
                    );
                } else {
                    LOG_INFO("Model refresh not supported for provider type: {}", static_cast<int>(e.provider_type));
                }
            }
            // ✅ 注意：不再在处理器中发布新的事件，避免无限循环
            // InfoPanelView 会直接监听这个事件并更新 UI
        }
    ));

    // 订阅 LLM 连接状态事件（也用于 LLM Studio 连接测试）
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::LLMConnectionStatusEvent>(
        [this](const Events::LLMConnectionStatusEvent& e) {
            if (!e.is_connected) {
                // is_connected = false 表示请求测试连接
                // ✅ 使用 TaskManager 异步处理连接测试，避免阻塞 UI
                if (e.provider_type == Events::LLMProviderType::LLMStudio) {
                    LOG_INFO("Scheduling async LLM Studio connection test to {}", e.base_url);
                    DearTs::Core::Tasks::TaskManager::instance().launch(
                        "Test LLM Studio Connection",
                        [this, base_url = e.base_url](const auto&) {
                            handle_llm_studio_connection_test(base_url);
                        },
                        DearTs::Core::Tasks::TaskType::Background
                    );
                } else if (e.provider_type == Events::LLMProviderType::Ollama) {
                    LOG_INFO("Scheduling async Ollama connection test to {}", e.base_url);
                    DearTs::Core::Tasks::TaskManager::instance().launch(
                        "Test Ollama Connection",
                        [this, base_url = e.base_url](const auto&) {
                            handle_ollama_connection_test(base_url);
                        },
                        DearTs::Core::Tasks::TaskType::Background
                    );
                } else {
                    // 云端 provider 不需要连接测试
                    LOG_INFO("Connection test not supported for provider type: {}", static_cast<int>(e.provider_type));
                }
            }
            // 否则，这是连接测试的结果，由 InfoPanelView 直接监听并更新 UI
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

    // 订阅 LLM 模型切换事件
    m_event_tokens.push_back(
        DearTs::Core::Event::EventBus::instance().subscribe<Events::LLMModelChangedEvent>(
            [this](const Events::LLMModelChangedEvent& e) {
                LOG_INFO("LLM model changed from {} to {}", e.old_model, e.new_model);

                // 从 ConfigManager 读取当前供应商配置
                DearTs::Core::Config::ConfigScope config("chat");

                std::string provider_id = config.get_or<std::string>("llm.provider", "ollama");
                std::string base_url = config.get_or<std::string>("llm.custom_base_url", "");
                std::string ollama_base_url = config.get_or<std::string>("llm.ollama_base_url", "http://localhost:11434");
                std::string llm_studio_base_url = config.get_or<std::string>("llm.llm_studio_base_url", "http://localhost:1234/v1");

                LOG_DEBUG("Model changed: provider_id='{}', base_url='{}', new_model='{}'", provider_id, base_url, e.new_model);

                // 根据供应商 ID 选择正确的 base URL
                if (provider_id == "ollama") {
                    if (base_url.empty()) {
                        base_url = ollama_base_url;
                    }
                    auto new_provider = LLM::LLMProviderFactory::create_ollama_provider(
                        base_url,
                        e.new_model
                    );
                    LLM::LLMManager::instance().set_provider(std::move(new_provider));
                    LOG_INFO("Ollama provider updated with model: {}", e.new_model);
                } else if (provider_id == "llmstudio") {
                    if (base_url.empty()) {
                        base_url = llm_studio_base_url;
                    }
                    auto new_provider = LLM::LLMProviderFactory::create_llm_studio_provider(
                        base_url,
                        e.new_model
                    );
                    LLM::LLMManager::instance().set_provider(std::move(new_provider));
                    LOG_INFO("LLM Studio provider updated with model: {}", e.new_model);
                } else {
                    // 其他供应商使用 HTTP provider
                    LOG_WARN("Unknown provider_id '{}', creating HTTP provider for model: {}", provider_id, e.new_model);

                    // 从 PRESET_LLM_PROVIDERS 获取默认 base_url
                    auto provider_it = std::find_if(
                        PRESET_LLM_PROVIDERS.begin(),
                        PRESET_LLM_PROVIDERS.end(),
                        [&](const LLMProviderConfig& cfg) { return cfg.id == provider_id; }
                    );

                    if (base_url.empty() && provider_it != PRESET_LLM_PROVIDERS.end()) {
                        base_url = provider_it->default_base_url;
                    }

                    auto new_provider = LLM::LLMProviderFactory::create_http_provider(base_url, "", e.new_model);
                    LLM::LLMManager::instance().set_provider(std::move(new_provider));
                    LOG_INFO("HTTP provider created for {} with model: {}", provider_id, e.new_model);
                }
            }
        )
    );

    // 订阅 LLM 提供商切换事件
    m_event_tokens.push_back(
        DearTs::Core::Event::EventBus::instance().subscribe<Events::LLMProviderChangedEvent>(
            [this](const Events::LLMProviderChangedEvent& e) {
                LOG_INFO("LLM provider changed from {} to {}", e.old_provider, e.new_provider);

                // 从 ConfigManager 读取供应商配置
                DearTs::Core::Config::ConfigScope config("chat");

                std::string base_url = config.get_or<std::string>("llm.custom_base_url", "");
                std::string model = config.get_or<std::string>("llm.model", "llama3.2");

                // 根据新供应商创建对应的 provider
                if (e.new_provider == "ollama") {
                    std::string ollama_base_url = config.get_or<std::string>("llm.ollama_base_url", "http://localhost:11434");
                    if (base_url.empty()) {
                        base_url = ollama_base_url;
                    }
                    auto new_provider = LLM::LLMProviderFactory::create_ollama_provider(base_url, model);
                    LLM::LLMManager::instance().set_provider(std::move(new_provider));
                    LOG_INFO("Created Ollama provider with model: {}", model);

                    // ✅ 切换到 Ollama 时，刷新模型列表
                    DearTs::Core::Event::EventBus::instance().publish(Events::LLMModelsUpdatedEvent{
                        .models = {},
                        .base_url = base_url,
                        .provider_type = Events::LLMProviderType::Ollama
                    });
                } else if (e.new_provider == "llmstudio") {
                    std::string llm_studio_base_url = config.get_or<std::string>("llm.llm_studio_base_url", "http://localhost:1234/v1");
                    if (base_url.empty()) {
                        base_url = llm_studio_base_url;
                    }
                    auto new_provider = LLM::LLMProviderFactory::create_llm_studio_provider(base_url, model);
                    LLM::LLMManager::instance().set_provider(std::move(new_provider));
                    LOG_INFO("Created LLM Studio provider with model: {}", model);

                    // ✅ 切换到 LLM Studio 时，刷新模型列表
                    DearTs::Core::Event::EventBus::instance().publish(Events::LLMModelsUpdatedEvent{
                        .models = {},
                        .base_url = base_url,
                        .provider_type = Events::LLMProviderType::LLMStudio
                    });
                } else {
                    // 其他供应商使用 HTTP provider
                    if (base_url.empty()) {
                        // 获取供应商的默认 base_url
                        auto provider_it = std::find_if(
                            PRESET_LLM_PROVIDERS.begin(),
                            PRESET_LLM_PROVIDERS.end(),
                            [&](const LLMProviderConfig& cfg) { return cfg.id == e.new_provider; }
                        );
                        if (provider_it != PRESET_LLM_PROVIDERS.end()) {
                            base_url = provider_it->default_base_url;
                        }
                    }
                    auto new_provider = LLM::LLMProviderFactory::create_http_provider(base_url, "", model);
                    LLM::LLMManager::instance().set_provider(std::move(new_provider));
                    LOG_INFO("Created HTTP provider for {} with model: {}", e.new_provider, model);
                }
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
            DearTs::Core::Event::EventBus::instance().publish(Events::LLMModelsUpdatedEvent{
                .models = models,
                .base_url = base_url,
                .provider_type = Events::LLMProviderType::Ollama
            });

            // 发布连接状态事件（成功）
            DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
                .is_connected = true,
                .base_url = base_url,
                .error_message = "",
                .provider_type = Events::LLMProviderType::Ollama
            });
        } else {
            // 模型列表为空，只发布连接状态但标记为未连接
            LOG_WARN("Ollama returned empty model list, might not be available");
            DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
                .is_connected = false,
                .base_url = base_url,
                .error_message = "No models available",
                .provider_type = Events::LLMProviderType::Ollama
            });
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get Ollama models: {}", e.what());

        // ✅ 首先发布模型列表更新事件（空列表 + 连接失败标志），确保刷新状态被重置
        DearTs::Core::Event::EventBus::instance().publish(Events::LLMModelsUpdatedEvent{
            .models = {},  // 空列表表示刷新失败
            .base_url = base_url,
            .provider_type = Events::LLMProviderType::Ollama
        });

        // 然后发布连接状态事件（失败）
        DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
            .is_connected = false,
            .base_url = base_url,
            .error_message = e.what(),
            .provider_type = Events::LLMProviderType::Ollama
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
    DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
        .is_connected = connected,
        .base_url = base_url,
        .error_message = connected ? "" : "Connection failed",
        .provider_type = Events::LLMProviderType::Ollama
    });
}

void ChatPlugin::handle_llm_studio_models_refresh(const std::string& base_url) {
    LOG_INFO("Refreshing LLM Studio models from {}", base_url);

    // 直接调用 LM Studio 的 /v1/models 端点
    try {
        using namespace DearTs::Core::Network;

        HttpClientConfig config;
        config.connect_timeout = std::chrono::seconds(10);
        config.request_timeout = std::chrono::seconds(30);

        BoostAsioHttpClient client(base_url, config);

        HttpRequest request;
        request.method = "GET";
        request.endpoint = "/v1/models";

        auto response_result = client.request(request);

        if (response_result.isErr()) {
            throw std::runtime_error(response_result.error());
        }

        auto response = response_result.unwrap();

        if (response.status_code != 200) {
            throw std::runtime_error(std::format("HTTP {}", response.status_code));
        }

        // 解析响应：OpenAI 兼容格式 {"data": [{"id": "model-name"}, ...]}
        std::vector<std::string> models;
        json response_json = json::parse(response.body);

        if (response_json.contains("data") && response_json["data"].is_array()) {
            for (const auto& item : response_json["data"]) {
                if (item.contains("id")) {
                    models.push_back(item["id"].get<std::string>());
                }
            }
        }

        LOG_INFO("Retrieved {} models from LLM Studio", models.size());

        if (!models.empty()) {
            // 发布模型列表更新事件
            DearTs::Core::Event::EventBus::instance().publish(Events::LLMModelsUpdatedEvent{
                .models = models,
                .base_url = base_url,
                .provider_type = Events::LLMProviderType::LLMStudio
            });

            // 发布连接状态事件（成功）
            DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
                .is_connected = true,
                .base_url = base_url,
                .error_message = "",
                .provider_type = Events::LLMProviderType::LLMStudio
            });
        } else {
            // 模型列表为空
            LOG_WARN("LLM Studio returned empty model list");
            DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
                .is_connected = false,
                .base_url = base_url,
                .error_message = "No models available",
                .provider_type = Events::LLMProviderType::LLMStudio
            });
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get LLM Studio models: {}", e.what());

        DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
            .is_connected = false,
            .base_url = base_url,
            .error_message = e.what(),
            .provider_type = Events::LLMProviderType::LLMStudio
        });
    }
}

void ChatPlugin::handle_llm_studio_connection_test(const std::string& base_url) {
    LOG_INFO("Testing LLM Studio connection to {}", base_url);

    // 直接测试 LM Studio 的 /v1/models 端点
    try {
        using namespace DearTs::Core::Network;

        HttpClientConfig config;
        config.connect_timeout = std::chrono::seconds(5);
        config.request_timeout = std::chrono::seconds(10);

        BoostAsioHttpClient client(base_url, config);

        HttpRequest request;
        request.method = "GET";
        request.endpoint = "/v1/models";  // LM Studio 使用 OpenAI 兼容 API

        auto response_result = client.request(request);
        bool connected = response_result.isOk() && response_result.unwrap().status_code == 200;

        LOG_INFO("LLM Studio connection test result: {}", connected ? "Connected" : "Failed");

        // 发布连接状态事件
        DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
            .is_connected = connected,
            .base_url = base_url,
            .error_message = connected ? "" : "Connection failed",
            .provider_type = Events::LLMProviderType::LLMStudio
        });

    } catch (const std::exception& e) {
        LOG_ERROR("LLM Studio connection test failed: {}", e.what());

        DearTs::Core::Event::EventBus::instance().publish(Events::LLMConnectionStatusEvent{
            .is_connected = false,
            .base_url = base_url,
            .error_message = e.what(),
            .provider_type = Events::LLMProviderType::LLMStudio
        });
    }
}

} // namespace DearTs::Plugins::Chat
