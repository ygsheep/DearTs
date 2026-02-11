/**
 * @file memory_core_plugin.cpp
 * @brief Memory Core 插件实现
 */

#include "memory_core/memory_core_plugin.hpp"
#include "memory_core/persistence/database.hpp"
#include "memory_core/memory/memory_manager.hpp"
#include "memory_core/memory/memory_extractor.hpp"
#include "memory_core/memory/llm_memory_extractor.hpp"
#include "memory_core/rag/rag_service.hpp"
#include "memory_core/rag/embedding_cache.hpp"
#include "memory_core/rag/embedding_provider.hpp"
#include "memory_core/rag/context_builder.hpp"
#include "memory_core/events/memory_events.hpp"
#include "chat/events/chat_events.hpp"
#include "core/config/config_manager.h"
#include "core/tasks/task_manager.h"
#include "liblogger/logger.h"
#include <random>
#include <filesystem>

namespace DearTs::Plugins::MemoryCore {

DearTs::Core::Result<void, std::string> MemoryCorePlugin::on_load() {
    LOG_INFO("Loading MemoryCore plugin...");

    // 1. 初始化数据库
    auto db_result = initialize_database();
    if (db_result.isErr()) {
        return DearTs::Core::Result<void, std::string>::err(
            "Database initialization failed: " + db_result.error()
        );
    }
    LOG_INFO("Database initialized");

    // 2. 初始化事件系统
    auto events_result = initialize_events();
    if (events_result.isErr()) {
        return DearTs::Core::Result<void, std::string>::err(
            "Events initialization failed: " + events_result.error()
        );
    }
    LOG_INFO("Event system initialized");

    // 3. 注册命令
    auto commands_result = register_commands();
    if (commands_result.isErr()) {
        return DearTs::Core::Result<void, std::string>::err(
            "Commands registration failed: " + commands_result.error()
        );
    }
    LOG_INFO("Commands registered");

    // 4. 初始化嵌入提供者和 RAG 服务
    // 从配置读取嵌入模型
    auto& config_manager = DearTs::Core::Config::ConfigManager::instance();
    DearTs::Core::Config::ConfigScope config("chat");

    // 读取当前使用的 LLM 提供商
    std::string provider_id = config.get_or<std::string>("llm.provider", "llmstudio");

    std::string base_url;
    std::string embed_model;

    if (provider_id == "llmstudio") {
        // LM Studio 配置
        base_url = config.get_or<std::string>("llm.llm_studio_base_url", "http://localhost:1234/v1");
        embed_model = config.get_or<std::string>("llm.embed_model", "text-embedding-nomic-embed-text-v1.5");
        LOG_INFO("Using LM Studio for embeddings: url={}, model={}", base_url, embed_model);
    } else if (provider_id == "ollama") {
        // Ollama 配置
        base_url = config.get_or<std::string>("llm.ollama_base_url", "http://localhost:11434");
        embed_model = config.get_or<std::string>("llm.embed_model", "nomic-embed-text");
        LOG_INFO("Using Ollama for embeddings: url={}, model={}", base_url, embed_model);
    } else {
        // 其他提供商使用 HTTP 模式
        base_url = config.get_or<std::string>("llm.custom_base_url", "");
        if (base_url.empty()) {
            // 尝试从预设提供商获取默认 URL
            // 简化处理：使用 LM Studio 作为默认
            base_url = "http://localhost:1234/v1";
        }
        embed_model = config.get_or<std::string>("llm.embed_model", "text-embedding-nomic-embed-text-v1.5");
        LOG_INFO("Using HTTP provider for embeddings: url={}, model={}", base_url, embed_model);
    }

    // 创建嵌入提供者（根据提供商类型）
    std::unique_ptr<RAG::IEmbeddingProvider> embedding_provider;

    if (provider_id == "ollama") {
        embedding_provider = RAG::EmbeddingProviderFactory::create_ollama_provider(
            base_url,
            embed_model
        );
    } else {
        // LM Studio 和其他提供商使用 HTTP 模式
        embedding_provider = RAG::EmbeddingProviderFactory::create_http_provider(
            base_url,
            embed_model,
            ""  // API Key（本地服务通常不需要）
        );
    }

    // 初始化 RAG 服务
    auto& rag_service = RAG::RAGService::instance();
    rag_service.initialize(RAG::CacheConfig::default_config(), std::move(embedding_provider));

    // 设置 MemoryManager 的嵌入提供者（创建一个新的）
    std::unique_ptr<RAG::IEmbeddingProvider> memory_embedding_provider;

    if (provider_id == "ollama") {
        memory_embedding_provider = RAG::EmbeddingProviderFactory::create_ollama_provider(
            base_url,
            embed_model
        );
    } else {
        memory_embedding_provider = RAG::EmbeddingProviderFactory::create_http_provider(
            base_url,
            embed_model,
            ""
        );
    }

    Memory::MemoryManager::instance().set_embedding_provider(std::move(memory_embedding_provider));

    LOG_INFO("RAG service and embedding provider initialized");

    // TODO: 后续阶段将实现
    // m_database = std::make_unique<Persistence::SQLiteDatabase>();
    // m_memoryManager = std::make_unique<Memory::MemoryManager>();
    // m_ragService = std::make_unique<RAG::RAGService>();
    // m_summarizer = std::make_unique<Summarizer::Summarizer>();
    // m_consistencyManager = std::make_unique<Consistency::ConsistencyManager>();

    LOG_INFO("MemoryCore plugin loaded successfully");
    return DearTs::Core::Result<void, std::string>::ok();
}

void MemoryCorePlugin::on_unload() {
    LOG_INFO("Unloading MemoryCore plugin...");

    // 清理事件监听（RAII 自动处理）
    m_event_tokens.clear();

    // TODO: 清理核心组件
    // m_consistencyManager.reset();
    // m_summarizer.reset();
    // m_ragService.reset();
    // m_memoryManager.reset();
    // m_database.reset();

    LOG_INFO("MemoryCore plugin unloaded");
}

void MemoryCorePlugin::on_enable() {
    LOG_INFO("MemoryCore plugin enabled");

    // TODO: 启动后台任务
    // - 心跳检测
    // - 离线队列处理
    // - 自动摘要触发
}

void MemoryCorePlugin::on_disable() {
    LOG_INFO("MemoryCore plugin disabled");

    // TODO: 停止后台任务
    // - 取消心跳检测
    // - 保存离线队列
    // - 保存待处理状态
}

DearTs::Core::Result<void, std::string> MemoryCorePlugin::initialize_database() {
#ifdef SQLITE3_FOUND
    // 1. 确定数据库路径
    std::string db_path;

    // 定义 Debug 模式检测宏（基于 _DEBUG 或 NDEBUG）
    #if defined(_DEBUG) || !defined(NDEBUG)
        #define DEARTS_DEBUG 1
    #else
        #define DEARTS_DEBUG 0
    #endif

    // 从 ConfigManager 获取数据库路径（如果存在）
    auto& config_manager = DearTs::Core::Config::ConfigManager::instance();

    // 使用 ConfigScope 读取配置
    DearTs::Core::Config::ConfigScope config("memory");

    if (DEARTS_DEBUG) {
        // Debug 模式：使用项目根目录的 data 文件夹
        db_path = config.get_or<std::string>("db_path", "D:/develop/CPlusPlus/Dear_SDL/DearTsd/data/memory.db");
        LOG_INFO("Debug mode detected, using project database path");
    } else {
        // Release 模式：使用相对路径
        db_path = config.get_or<std::string>("db_path", "data/memory.db");
        LOG_INFO("Release mode detected, using relative database path");
    }

    // 2. 确保数据库目录存在
    std::filesystem::path db_file(db_path);
    if (db_file.has_parent_path()) {
        std::filesystem::create_directories(db_file.parent_path());
        LOG_INFO("Created database directory: {}", db_file.parent_path().string());
    }

    // 3. 初始化 SQLiteDatabase
    LOG_INFO("Initializing database: {}", db_path);
    auto& db = Persistence::SQLiteDatabase::instance();
    auto init_result = db.initialize(db_path);
    if (init_result.isErr()) {
        return DearTs::Core::Result<void, std::string>::err(
            "Failed to initialize database: " + init_result.error()
        );
    }

    LOG_INFO("Database initialized successfully");
    return DearTs::Core::Result<void, std::string>::ok();
#else
    // SQLite3 未找到时跳过数据库初始化
    LOG_WARN("SQLite3 not available, database features will be disabled");
    LOG_INFO("Database initialization skipped (SQLITE3_FOUND not defined)");
    return DearTs::Core::Result<void, std::string>::ok();
#endif
}

DearTs::Core::Result<void, std::string> MemoryCorePlugin::initialize_events() {
    using namespace Events;

    // 获取事件总线
    auto& event_bus = DearTs::Core::Event::EventBus::instance();

    // ✅ 订阅 Chat 插件的事件
    // - Chat::ConversationCreatedEvent -> 将会话同步到数据库
    // - Chat::ConversationDeletedEvent -> 从数据库删除会话和相关数据
    // - Chat::ConversationUpdatedEvent -> 更新数据库中的会话信息（如标题重命名）

    // 订阅会话创建事件
    auto token0 = event_bus.subscribe<Chat::Events::ConversationCreatedEvent>(
        [this](const Chat::Events::ConversationCreatedEvent& e) {
            this->handle_conversation_created(e);
        }
    );
    m_event_tokens.push_back(std::move(token0));

    // 订阅会话删除事件
    auto token1 = event_bus.subscribe<Chat::Events::ConversationDeletedEvent>(
        [this](const Chat::Events::ConversationDeletedEvent& e) {
            this->handle_conversation_deleted(e);
        }
    );
    m_event_tokens.push_back(std::move(token1));

    // 订阅会话更新事件（用于标题重命名等）
    auto token2 = event_bus.subscribe<Chat::Events::ConversationUpdatedEvent>(
        [this](const Chat::Events::ConversationUpdatedEvent& e) {
            this->handle_conversation_updated(e);
        }
    );
    m_event_tokens.push_back(std::move(token2));

    // 订阅 Memory Core 插件内部事件
    auto token3 = event_bus.subscribe<MessageSaveRequestedEvent>(
        [this](const MessageSaveRequestedEvent& e) {
            this->handle_message_save_requested(e);
        }
    );
    m_event_tokens.push_back(std::move(token3));

    auto token4 = event_bus.subscribe<RAGQueryRequestedEvent>(
        [this](const RAGQueryRequestedEvent& e) {
            this->handle_rag_query_requested(e);
        }
    );
    m_event_tokens.push_back(std::move(token4));

    auto token5 = event_bus.subscribe<MemoryExtractRequestedEvent>(
        [this](const MemoryExtractRequestedEvent& e) {
            this->handle_memory_extract_requested(e);
        }
    );
    m_event_tokens.push_back(std::move(token5));

    LOG_INFO("Event system initialized: {} internal subscriptions", m_event_tokens.size());
    return DearTs::Core::Result<void, std::string>::ok();
}

// ============ 事件处理器实现 ============

void MemoryCorePlugin::handle_message_save_requested(const Events::MessageSaveRequestedEvent& event) {
    LOG_INFO("[DB SAVE] Handling message save request: conv_id={}, uuid={}, role='{}', content='{}...' (len={})",
             event.conversation_id, event.message_uuid, event.role,
             event.content.substr(0, std::min(size_t(30), event.content.length())),
             event.content.length());

    auto& db = Persistence::SQLiteDatabase::instance();
    auto& event_bus = DearTs::Core::Event::EventBus::instance();

    Events::MessageSavedEvent saved_event;
    saved_event.init_base("memory_core");
    saved_event.conversation_id = event.conversation_id;
    saved_event.message_uuid = event.message_uuid;

    // 1. 确保会话存在于数据库
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    auto conv_result = db.insert_conversation(
        event.conversation_id,
        event.conversation_title.empty() ? "新对话" : event.conversation_title,
        "chat",
        timestamp
    );

    if (conv_result.isErr()) {
        saved_event.success = false;
        saved_event.error_message = "Failed to save conversation: " + conv_result.error();
        event_bus.publish(saved_event);
        LOG_ERROR("Failed to save conversation: {}", conv_result.error());
        return;
    }

    // 2. 保存消息到数据库
    // 注意：tokens >= 0 时才传递，否则传递 std::nullopt
    auto tokens_opt = event.tokens >= 0 ? std::optional<int>(event.tokens) : std::nullopt;

    auto msg_result = db.insert_message(
        event.conversation_id,
        event.message_uuid,
        event.role,
        event.content,
        timestamp,
        tokens_opt
    );

    if (msg_result.isErr()) {
        saved_event.success = false;
        saved_event.error_message = "Failed to save message: " + msg_result.error();
        event_bus.publish(saved_event);
        LOG_ERROR("Failed to save message: {}", msg_result.error());
        return;
    }

    // 3. 成功保存
    saved_event.database_id = msg_result.unwrap();
    saved_event.success = true;
    saved_event.error_message.clear();
    event_bus.publish(saved_event);

    LOG_INFO("Message saved successfully: id={}, uuid={}",
             saved_event.database_id, event.message_uuid);
}

void MemoryCorePlugin::handle_rag_query_requested(const Events::RAGQueryRequestedEvent& event) {
    LOG_INFO("Handling RAG query: query='{}', max_results={}, min_similarity={}",
             event.query, event.max_results, event.min_similarity);

    auto& event_bus = DearTs::Core::Event::EventBus::instance();
    Events::RAGQueryCompletedEvent completed_event;
    completed_event.init_base("memory_core");
    completed_event.query = event.query;

    // 获取 RAG 服务
    auto& rag_service = RAG::RAGService::instance();

    // 确保已初始化
    static bool rag_initialized = false;
    if (!rag_initialized) {
        rag_service.initialize();
        rag_initialized = true;
    }

    // 构建查询选项
    // 从配置读取嵌入模型
    DearTs::Core::Config::ConfigScope config("chat");
    std::string embed_model = config.get_or<std::string>("llm.embed_model", "nomic-embed-text");

    RAG::RAGQueryOptions options;
    options.max_results = event.max_results;
    options.min_similarity = event.min_similarity;
    options.embedding_model = embed_model;

    // 执行查询
    auto query_result = rag_service.query(event.query, options);

    if (query_result.isErr()) {
        completed_event.success = false;
        completed_event.error_message = query_result.error();
        completed_event.results.clear();
    } else {
        // 转换结果格式
        auto& rag_results = query_result.unwrap();
        for (const auto& rag_result : rag_results) {
            Events::RAGResultItem item;
            item.content = rag_result.memory.content;
            if (rag_result.memory.source_conversation_id.has_value()) {
                item.source_conversation_id = rag_result.memory.source_conversation_id.value();
            }
            item.similarity = rag_result.similarity;
            item.memory_type = Memory::Memory::type_to_string(rag_result.memory.type);
            item.timestamp = rag_result.memory.created_at;

            completed_event.results.push_back(item);
        }

        completed_event.success = true;
        completed_event.error_message.clear();
    }

    LOG_INFO("RAG query completed: {} results", completed_event.results.size());
    event_bus.publish(completed_event);
}

void MemoryCorePlugin::handle_memory_extract_requested(const Events::MemoryExtractRequestedEvent& event) {
    LOG_INFO("Handling memory extract request: conversation_id={}, message_count={}, use_llm={}",
             event.conversation_id, event.message_contents.size(), event.use_llm);

    auto& event_bus = DearTs::Core::Event::EventBus::instance();

    if (event.use_llm) {
        // 使用 LLM 提取（后台任务，避免阻塞 UI）
        LOG_INFO("Using LLM-based extraction (background task)");

        auto& llm_extractor = LLMMemoryExtractor::instance();

        // 确保 LLM 提取器已初始化
        static bool llm_initialized = false;
        if (!llm_initialized) {
            // 从配置读取模型，而不是使用硬编码的默认值
            auto& config_manager = DearTs::Core::Config::ConfigManager::instance();
            DearTs::Core::Config::ConfigScope config("chat");

            std::string model = config.get_or<std::string>("llm.model", "qwen2.5-coder:1.5b");
            std::string base_url = config.get_or<std::string>("llm.custom_base_url", "");
            std::string ollama_base_url = config.get_or<std::string>("llm.ollama_base_url", "http://localhost:11434");

            // 如果 custom_base_url 为空，使用向后兼容的 ollama_base_url
            if (base_url.empty()) {
                base_url = ollama_base_url;
            }

            // 创建配置
            LLMExtractorConfig extractor_config;
            extractor_config.ollama_url = base_url;
            extractor_config.chat_model = model;
            extractor_config.embed_model = "nomic-embed-text";
            extractor_config.temperature = 0.3;
            extractor_config.max_tokens = 500;
            extractor_config.enable_verification = true;

            LOG_INFO("Initializing LLM extractor with model: {} (from config)", model);

            auto init_result = llm_extractor.initialize(extractor_config);
            if (init_result.isOk()) {
                llm_initialized = true;
            } else {
                LOG_WARN("Failed to initialize LLM extractor: {}, falling back to rule-based",
                         init_result.error().c_str());
            }
        }

        if (llm_initialized) {
            // 启动后台任务进行 LLM 提取
            auto& task_manager = DearTs::Core::Tasks::TaskManager::instance();

            task_manager.launch(
                "LLM Memory Extraction",
                [this, event, &llm_extractor](const std::atomic<bool>& should_cancel) {
                    LOG_INFO("LLM extraction task started for conversation {}", event.conversation_id);

                    // 检查是否已取消
                    if (should_cancel.load()) {
                        LOG_INFO("LLM extraction task cancelled before starting");
                        return;
                    }

                    // 使用 LLM 从消息中提取记忆
                    auto llm_result = llm_extractor.extract_from_messages(
                        event.message_contents,
                        event.conversation_id
                    );

                    std::vector<Events::ExtractedMemory> extracted_memories;

                    if (llm_result.isOk() && !should_cancel.load()) {
                        for (const auto& extraction : llm_result.unwrap()) {
                            // 保存到数据库
                            auto& manager = Memory::MemoryManager::instance();
                            auto add_result = manager.add_memory(extraction.memory);

                            Events::ExtractedMemory extracted;
                            extracted.type = Memory::Memory::type_to_string(extraction.memory.type);
                            extracted.content = extraction.memory.content;
                            extracted.importance = extraction.memory.importance;

                            extracted_memories.push_back(extracted);
                        }

                        LOG_INFO("LLM extracted {} memories", extracted_memories.size());
                    } else {
                        if (llm_result.isErr()) {
                            LOG_WARN("LLM extraction failed: {}", llm_result.error().c_str());
                        }
                        // LLM 失败或取消，使用规则匹配作为后备
                        LOG_INFO("Falling back to rule-based extraction");
                        extracted_memories = perform_rule_based_extraction(event);
                    }

                    // 发布内存提取完成事件
                    auto& event_bus = DearTs::Core::Event::EventBus::instance();
                    Events::MemoryExtractedEvent extracted_event;
                    extracted_event.init_base("memory_core");
                    extracted_event.conversation_id = event.conversation_id;
                    extracted_event.memories = extracted_memories;
                    extracted_event.success = true;
                    extracted_event.error_message.clear();

                    LOG_INFO("Extracted {} memories total", extracted_memories.size());
                    event_bus.publish(extracted_event);
                },
                DearTs::Core::Tasks::TaskType::Background  // 后台任务，不影响 UI
            );

            // 立即返回，不等待任务完成
            LOG_INFO("LLM extraction task launched for conversation {}", event.conversation_id);
            return;
        }
    }

    // 如果 LLM 提取未启用或失败，使用规则匹配作为后备（同步，很快）
    LOG_INFO("Using rule-based extraction (synchronous)");
    auto extracted_memories = perform_rule_based_extraction(event);

    // 发布内存提取完成事件
    Events::MemoryExtractedEvent extracted_event;
    extracted_event.init_base("memory_core");
    extracted_event.conversation_id = event.conversation_id;
    extracted_event.memories = extracted_memories;
    extracted_event.success = true;
    extracted_event.error_message.clear();

    LOG_INFO("Extracted {} memories total", extracted_memories.size());
    event_bus.publish(extracted_event);
}

// 辅助函数：执行基于规则的记忆提取
std::vector<Events::ExtractedMemory> MemoryCorePlugin::perform_rule_based_extraction(
    const Events::MemoryExtractRequestedEvent& event) {

    std::vector<Events::ExtractedMemory> extracted_memories;
    auto& extractor = Memory::MemoryExtractor::instance();

    // 确保规则已初始化
    static bool rules_initialized = false;
    if (!rules_initialized) {
        extractor.initialize_default_rules();
        rules_initialized = true;
    }

    // 使用规则匹配提取记忆
    for (const auto& message : event.message_contents) {
        auto extract_result = extractor.extract_from_message(
            message,
            event.conversation_id
        );

        if (extract_result.isOk()) {
            for (const auto& extraction : extract_result.unwrap()) {
                // 保存到数据库
                auto& manager = Memory::MemoryManager::instance();
                auto add_result = manager.add_memory(extraction.memory);

                Events::ExtractedMemory extracted;
                extracted.type = Memory::Memory::type_to_string(extraction.memory.type);
                extracted.content = extraction.memory.content;
                extracted.importance = extraction.memory.importance;
                if (extraction.memory.source_message_id.has_value()) {
                    extracted.source_message_id = std::to_string(extraction.memory.source_message_id.value());
                }

                extracted_memories.push_back(extracted);
            }
        }
    }

    return extracted_memories;
}

DearTs::Core::Result<void, std::string> MemoryCorePlugin::register_commands() {
    // TODO: 注册全局命令
    // - memory_core.save_message
    // - memory_core.rag_query
    // - memory_core.extract_memory
    // - memory_core.generate_summary

    LOG_INFO("Commands registration placeholder");
    return DearTs::Core::Result<void, std::string>::ok();
}

// ============ Chat 事件处理器实现 ============

void MemoryCorePlugin::handle_conversation_created(
    const DearTs::Plugins::Chat::Events::ConversationCreatedEvent& event
) {
    LOG_INFO("Handling conversation created: conv_id={}, title={}, type={}",
             event.conversation_id, event.title, static_cast<int>(event.type));

    auto& db = Persistence::SQLiteDatabase::instance();

    // 获取当前时间戳
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    // 将会话保存到数据库
    auto result = db.insert_conversation(
        event.conversation_id,
        event.title,
        "chat",  // 简化类型名称
        timestamp
    );

    if (result.isErr()) {
        LOG_ERROR("Failed to save conversation to database: {}", result.error());
    } else {
        LOG_INFO("Conversation saved to database: conv_id={}, title={}",
                 event.conversation_id, event.title);
    }
}

void MemoryCorePlugin::handle_conversation_deleted(
    const DearTs::Plugins::Chat::Events::ConversationDeletedEvent& event
) {
    LOG_INFO("Handling conversation deleted: conv_id={}", event.conversation_id);

    try {
        LOG_INFO("Getting database instance...");
        auto& db = Persistence::SQLiteDatabase::instance();

        LOG_INFO("Calling delete_conversation...");
        // 从数据库删除会话（由于外键约束，相关消息会自动删除）
        auto result = db.delete_conversation(event.conversation_id);

        LOG_INFO("delete_conversation returned...");
        if (result.isErr()) {
            // 不记录完整的错误消息，因为它可能太长
            LOG_ERROR("Failed to delete conversation from database");
        } else {
            LOG_INFO("Conversation deleted from database: conv_id={}", event.conversation_id);
        }
    } catch (const std::exception& e) {
        // 捕获任何异常，避免传播到 EventBus
        LOG_ERROR("Exception in handle_conversation_deleted");
    } catch (...) {
        // 捕获所有其他异常
        LOG_ERROR("Unknown exception in handle_conversation_deleted");
    }
}

void MemoryCorePlugin::handle_conversation_updated(
    const DearTs::Plugins::Chat::Events::ConversationUpdatedEvent& event
) {
    LOG_INFO("Handling conversation updated: conv_id={}, type={}",
             event.conversation_id,
             static_cast<int>(event.update_type));

    auto& db = Persistence::SQLiteDatabase::instance();

    // 处理标题变更
    if (event.update_type == DearTs::Plugins::Chat::Events::ConversationUpdateType::TitleChanged) {
        auto result = db.update_conversation_title(event.conversation_id, event.new_title);

        if (result.isErr()) {
            LOG_ERROR("Failed to update conversation title in database: {}", result.error());
        } else {
            LOG_INFO("Conversation title updated in database: conv_id={}, new_title={}",
                     event.conversation_id, event.new_title);
        }
    }
}

} // namespace DearTs::Plugins::MemoryCore
