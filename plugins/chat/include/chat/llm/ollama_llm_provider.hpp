/**
 * @file ollama_llm_provider.hpp
 * @brief Ollama LLM 提供商
 * @details 支持本地 Ollama 服务的 LLM Provider，支持流式输出 (NDJSON)
 */

#pragma once

#include "chat/llm/llm_interface.hpp"
#include "core/result.h"
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>

namespace DearTs::Core::Network {
    class BoostAsioHttpClient;
}

namespace DearTs::Plugins::Chat::LLM {

/**
 * @brief 向量嵌入结果
 */
struct EmbeddingResult {
    std::string model;                           ///< 模型名称
    std::vector<float> embedding;                ///< 嵌入向量
    int dimension;                                ///< 向量维度
    int64_t total_duration_ms;                   ///< 总耗时（毫秒）
    int64_t load_duration_ms;                    ///< 模型加载耗时（毫秒）
    int64_t prompt_eval_count;                   ///< 提示词评估计数

    /**
     * @brief 序列化为 JSON
     */
    std::string to_json() const;

    /**
     * @brief 从 JSON 解析
     */
    static DearTs::Core::Result<EmbeddingResult, std::string> from_json(
        const std::string& json
    );
};

/**
 * @brief Ollama LLM 提供商
 * @details 连接到本地 Ollama 服务 (http://localhost:11434)
 *          支持 NDJSON 流式输出和模型管理
 */
class OllamaLLMProvider : public ILLMProvider {
public:
    /**
     * @brief 构造函数
     * @param base_url Ollama 服务地址 (默认: http://localhost:11434)
     * @param model 默认模型名称 (默认: llama3.2)
     */
    explicit OllamaLLMProvider(
        const std::string& base_url = "http://localhost:11434",
        const std::string& model = "llama3.2"
    );

    ~OllamaLLMProvider() override = default;

    // ========== ILLMProvider 接口实现 ==========

    [[nodiscard]] std::string get_name() const override { return "Ollama"; }

    [[nodiscard]] bool is_available() const override;

    [[nodiscard]] std::shared_ptr<Core::Tasks::Task> send_async(
        const LLMRequest& request,
        std::function<void(const LLMResponse&)> callback
    ) override;

    [[nodiscard]] DearTs::Core::Result<LLMResponse, std::string> send(
        const LLMRequest& request
    ) override;

    [[nodiscard]] std::vector<std::string> get_models() const override;

    // ========== Embed API ==========

    /**
     * @brief 生成文本嵌入
     * @param text 输入文本
     * @param model 嵌入模型名称（默认: nomic-embed-text）
     * @return 嵌入结果或错误信息
     */
    [[nodiscard]] DearTs::Core::Result<EmbeddingResult, std::string> generate_embedding(
        const std::string& text,
        const std::string& model = "nomic-embed-text"
    ) const;

    /**
     * @brief 批量生成文本嵌入
     * @param texts 输入文本列表
     * @param model 嵌入模型名称
     * @return 嵌入结果列表或错误信息
     */
    [[nodiscard]] DearTs::Core::Result<std::vector<EmbeddingResult>, std::string> generate_embeddings_batch(
        const std::vector<std::string>& texts,
        const std::string& model = "nomic-embed-text"
    ) const;

    /**
     * @brief 获取可用的嵌入模型列表
     * @return 嵌入模型名称列表
     */
    [[nodiscard]] std::vector<std::string> get_embedding_models() const;

private:
    // ========== 请求构建 ==========

    /**
     * @brief 构建聊天 API 请求 JSON
     */
    std::string build_chat_request(const LLMRequest& request) const;

    /**
     * @brief 构建生成 API 请求 JSON（用于代码模型）
     */
    std::string build_generate_request(const LLMRequest& request) const;

    /**
     * @brief 构建嵌入 API 请求 JSON
     */
    std::string build_embedding_request(const std::string& text, const std::string& model) const;

    // ========== 响应解析 ==========

    /**
     * @brief 解析 NDJSON 响应（非流式）
     */
    DearTs::Core::Result<LLMResponse, std::string> parse_response(
        const std::string& ndjson_body
    ) const;

    /**
     * @brief 解析单行 NDJSON
     * @return 提取的内容文本，如果解析失败返回空字符串
     */
    std::string parse_ndjson_line(const std::string& line) const;

    /**
     * @brief 解析嵌入响应
     */
    DearTs::Core::Result<EmbeddingResult, std::string> parse_embedding_response(
        const std::string& json_body
    ) const;

    // ========== 流式处理 ==========

    /**
     * @brief 发送流式请求
     * @param request LLM 请求
     * @param on_chunk 内容块回调
     * @return 成功返回 Result::ok()，失败返回错误信息
     */
    DearTs::Core::Result<void, std::string> send_streaming_request(
        const LLMRequest& request,
        const std::function<void(const std::string&)>& on_chunk
    ) const;

    // ========== HTTP 请求 ==========

    /**
     * @brief 发送 HTTP 请求
     * @param endpoint API 端点 (如 /api/chat, /api/tags)
     * @param json_body 请求 JSON 正文
     * @param on_stream_chunk 流式数据回调（可选）
     * @return 响应内容字符串或错误
     */
    DearTs::Core::Result<std::string, std::string> send_http_request(
        const std::string& endpoint,
        const std::string& json_body,
        const std::function<void(const std::string&)>& on_stream_chunk = nullptr
    ) const;

    /**
     * @brief 测试连接
     */
    bool test_connection() const;

    /**
     * @brief 获取或创建 HTTP 客户端（线程安全）
     * @details 使用延迟初始化，避免在构造函数中创建 IO 线程
     *          使用双重检查锁定模式优化性能
     */
    [[nodiscard]] DearTs::Core::Network::BoostAsioHttpClient* get_client() const;

    // ========== 成员变量 ==========

    std::string m_base_url;  // Ollama 服务地址
    mutable std::unique_ptr<DearTs::Core::Network::BoostAsioHttpClient> m_client;  // HTTP 客户端（延迟初始化）
    mutable std::mutex m_client_mutex;  // 保护 m_client 的互斥锁
};

} // namespace DearTs::Plugins::Chat::LLM
