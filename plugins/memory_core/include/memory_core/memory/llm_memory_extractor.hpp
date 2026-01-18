/**
 * @file llm_memory_extractor.hpp
 * @brief LLM 记忆提取器 - 使用 LLM 从对话中提取记忆
 *
 * 功能：
 * - 使用 LLM 提取结构化记忆
 * - 支持多种记忆类型
 * - 提示词模板
 * - 结果验证
 */

#pragma once

#include "core/result.h"
#include "memory_core/memory/memory_manager.hpp"
#include "memory_core/memory/memory_extractor.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <mutex>

namespace DearTs::Plugins::MemoryCore {

// 前向声明 Ollama LLM Provider（来自 Chat 插件）
namespace LLMIntegration {
    struct LLMExtractResult;
    class OllamaLLMClient;
}

/**
 * @brief LLM 提取配置
 */
struct LLMExtractorConfig {
    std::string ollama_url;                     ///< Ollama 服务 URL
    std::string chat_model;                      ///< 聊天模型
    std::string embed_model;                     ///< 嵌入模型
    double temperature;                         ///< 温度参数
    int max_tokens;                             ///< 最大 token 数
    bool enable_verification;                   ///< 启用结果验证

    /**
     * @brief 默认配置
     */
    static LLMExtractorConfig default_config() {
        return LLMExtractorConfig{
            .ollama_url = "http://localhost:11434",
            .chat_model = "llama3.2",
            .embed_model = "nomic-embed-text",
            .temperature = 0.3,
            .max_tokens = 500,
            .enable_verification = true
        };
    }
};

/**
 * @brief LLM 提取统计
 */
struct LLMExtractorStats {
    int total_extractions;                      ///< 总提取次数
    int successful_extractions;                 ///< 成功提取次数
    int failed_extractions;                     ///< 失败提取次数
    int64_t total_duration_ms;                  ///< 总耗时（毫秒）
    double average_duration_ms;                 ///< 平均耗时

    /**
     * @brief 创建空统计
     */
    static LLMExtractorStats empty() {
        return LLMExtractorStats{
            .total_extractions = 0,
            .successful_extractions = 0,
            .failed_extractions = 0,
            .total_duration_ms = 0,
            .average_duration_ms = 0.0
        };
    }
};

/**
 * @brief LLM 记忆提取器
 *
 * 使用 LLM 从对话中智能提取记忆
 */
class LLMMemoryExtractor {
public:
    /**
     * @brief 获取单例实例
     */
    static LLMMemoryExtractor& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    LLMMemoryExtractor(const LLMMemoryExtractor&) = delete;
    LLMMemoryExtractor& operator=(const LLMMemoryExtractor&) = delete;

    // ============ 初始化 ============

    /**
     * @brief 初始化 LLM 提取器
     * @param config 提取器配置
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> initialize(
        const LLMExtractorConfig& config = LLMExtractorConfig::default_config()
    );

    /**
     * @brief 关闭 LLM 提取器
     */
    void shutdown();

    // ============ 记忆提取 ============

    /**
     * @brief 从对话中提取记忆
     * @param user_message 用户消息
     * @param assistant_response 助手回复
     * @param conversation_id 会话 ID
     * @return 提取结果列表或错误信息
     */
    DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string> extract_from_conversation(
        const std::string& user_message,
        const std::string& assistant_response,
        const std::string& conversation_id = ""
    );

    /**
     * @brief 从多条消息中提取记忆
     * @param messages 消息列表（格式: "role: content"）
     * @param conversation_id 会话 ID
     * @return 提取结果列表或错误信息
     */
    DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string> extract_from_messages(
        const std::vector<std::string>& messages,
        const std::string& conversation_id = ""
    );

    /**
     * @brief 批量提取记忆
     * @param conversations 对话列表（每条包含用户消息和助手回复）
     * @param conversation_id 会话 ID
     * @return 提取结果列表或错误信息
     */
    DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string> extract_batch(
        const std::vector<std::pair<std::string, std::string>>& conversations,
        const std::string& conversation_id = ""
    );

    // ============ 统计 ============

    /**
     * @brief 获取统计信息
     * @return 统计数据
     */
    LLMExtractorStats get_stats() const;

    /**
     * @brief 打印统计信息
     */
    void log_stats() const;

    // ============ 配置 ============

    /**
     * @brief 获取配置
     * @return 当前配置
     */
    const LLMExtractorConfig& get_config() const {
        return m_config;
    }

    /**
     * @brief 更新配置
     * @param config 新配置
     */
    void set_config(const LLMExtractorConfig& config);

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    LLMMemoryExtractor() = default;

    /**
     * @brief 析构函数
     */
    ~LLMMemoryExtractor() = default;

    /**
     * @brief 构建提取提示词
     */
    std::string build_extraction_prompt(
        const std::string& user_message,
        const std::string& assistant_response
    );

    /**
     * @brief 解析 LLM 响应
     */
    DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string> parse_llm_response(
        const std::string& response,
        const std::string& conversation_id
    );

    /**
     * @brief 验证提取结果
     */
    bool validate_extraction(const Memory::ExtractionResult& result);

    /**
     * @brief 更新统计
     */
    void update_stats(bool success, int64_t duration_ms);

    /**
     * @brief 检查 LLM 可用性
     */
    bool check_llm_available();

    // ============ 成员变量 ============

    LLMExtractorConfig m_config;                         ///< 配置
    LLMExtractorStats m_stats;                            ///< 统计
    std::unique_ptr<LLMIntegration::OllamaLLMClient> m_llm_client;  ///< LLM 客户端
    bool m_initialized;                                   ///< 是否已初始化
    mutable std::mutex m_mutex;                           ///< 互斥锁
};

} // namespace DearTs::Plugins::MemoryCore
