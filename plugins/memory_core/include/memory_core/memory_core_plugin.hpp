/**
 * @file memory_core_plugin.hpp
 * @brief Memory Core 插件入口 - 提供持久化、记忆管理和 RAG 服务
 */

#pragma once

#include "core/plugin/plugin.h"
#include "core/event/event_bus.h"
#include <memory>
#include <vector>

// Chat 插件事件前向声明（全局命名空间）
namespace DearTs { namespace Plugins { namespace Chat { namespace Events {
    struct ConversationDeletedEvent;
    struct ConversationUpdatedEvent;
    struct ConversationCreatedEvent;
}}}}

namespace DearTs::Plugins::MemoryCore {

// ============ 事件类型前向声明 ============
namespace Events {
    struct MessageSaveRequestedEvent;
    struct RAGQueryRequestedEvent;
    struct MemoryExtractRequestedEvent;
    struct SummaryRequestedEvent;
    struct ExtractedMemory;
    struct MemoryExtractedEvent;
} // namespace Events

// ============ 其他类型前向声明 ============
namespace Memory {
    class MemoryManager;
    class MemoryExtractor;
}

class LLMMemoryExtractor;

namespace RAG {
    class RAGService;
    class EmbeddingCache;
    class ContextBuilder;
}

/**
 * @brief Memory Core 插件主类
 *
 * 提供：
 * - SQLite 数据库持久化
 * - 跨会话全局记忆管理
 * - RAG 语义检索
 * - 会话摘要生成
 * - 事件驱动集成
 */
class MemoryCorePlugin : public Core::Plugin::IPlugin {
public:
    MemoryCorePlugin() = default;
    ~MemoryCorePlugin() override = default;

    // ============ IPlugin 接口实现 ============

    [[nodiscard]] Core::Plugin::PluginInfo get_info() const override {
        return Core::Plugin::PluginInfo{
            .name = "memory_core",
            .author = "DearTs Team",
            .description = "Core memory persistence and RAG service",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    [[nodiscard]] DearTs::Core::Result<void, std::string> on_load() override;
    void on_unload() override;
    void on_enable() override;
    void on_disable() override;

    // ============ 公共接口（供其他插件使用） ============

    // 注意：具体的服务类将在后续阶段实现
    // 这里先声明接口，实现时再添加

private:
    // ============ 初始化方法 ============

    /**
     * @brief 初始化数据库
     */
    [[nodiscard]] DearTs::Core::Result<void, std::string> initialize_database();

    /**
     * @brief 初始化事件系统
     */
    [[nodiscard]] DearTs::Core::Result<void, std::string> initialize_events();

    /**
     * @brief 注册命令
     */
    [[nodiscard]] DearTs::Core::Result<void, std::string> register_commands();

    // ============ 事件处理器 ============

    /**
     * @brief 处理消息保存请求
     */
    void handle_message_save_requested(const Events::MessageSaveRequestedEvent& event);

    /**
     * @brief 处理 RAG 查询请求
     */
    void handle_rag_query_requested(const Events::RAGQueryRequestedEvent& event);

    /**
     * @brief 处理记忆提取请求
     */
    void handle_memory_extract_requested(const Events::MemoryExtractRequestedEvent& event);

    // ============ Chat 事件处理器 ============

    /**
     * @brief 处理会话删除事件
     */
    void handle_conversation_deleted(const DearTs::Plugins::Chat::Events::ConversationDeletedEvent& event);

    /**
     * @brief 处理会话更新事件（标题重命名等）
     */
    void handle_conversation_updated(const DearTs::Plugins::Chat::Events::ConversationUpdatedEvent& event);

    /**
     * @brief 处理会话创建事件
     */
    void handle_conversation_created(const DearTs::Plugins::Chat::Events::ConversationCreatedEvent& event);

    /**
     * @brief 处理会话摘要请求（TODO: 阶段 6）
     */
    // void handle_summary_requested(const Events::SummaryRequestedEvent& event);

    // ============ 辅助方法 ============

    /**
     * @brief 执行基于规则的记忆提取（同步，快速）
     */
    std::vector<Events::ExtractedMemory> perform_rule_based_extraction(
        const Events::MemoryExtractRequestedEvent& event);

    // ============ 成员变量 ============

    // 核心组件（后续阶段实现）
    // std::unique_ptr<Persistence::SQLiteDatabase> m_database;
    // std::unique_ptr<Memory::MemoryManager> m_memoryManager;
    // std::unique_ptr<RAG::RAGService> m_ragService;
    // std::unique_ptr<Summarizer::Summarizer> m_summarizer;
    // std::unique_ptr<Consistency::ConsistencyManager> m_consistencyManager;

    // 事件订阅 Token（RAII 自动清理）
    std::vector<DearTs::Core::Event::EventToken> m_event_tokens;
};

} // namespace DearTs::Plugins::MemoryCore
