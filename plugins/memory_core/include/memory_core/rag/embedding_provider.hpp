/**
 * @file embedding_provider.hpp
 * @brief 嵌入向量提供者接口 - 连接 MemoryCore 和 LLM Provider
 *
 * 功能：
 * - 定义统一的嵌入生成接口
 * - 支持多种 LLM 提供商（Ollama、OpenAI 等）
 * - 提供批量嵌入生成功能
 */

#pragma once

#include "core/result.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace DearTs::Plugins::MemoryCore::RAG {

/**
 * @brief 嵌入向量结果
 */
struct EmbeddingVector {
    std::vector<float> data;      ///< 向量数据
    std::string model;            ///< 使用的模型名称
    int dimension;                ///< 向量维度
    int64_t duration_ms;          ///< 生成耗时（毫秒）

    /**
     * @brief 计算向量大小（字节）
     */
    size_t size_bytes() const {
        return data.size() * sizeof(float);
    }

    /**
     * @brief 序列化为 JSON 字符串（用于数据库存储）
     */
    std::string to_json() const;

    /**
     * @brief 从 JSON 字符串解析
     */
    static DearTs::Core::Result<EmbeddingVector, std::string> from_json(
        const std::string& json_str
    );
};

/**
 * @brief 嵌入向量提供者接口
 *
 * 定义统一的嵌入生成接口，支持多种 LLM 提供商
 */
class IEmbeddingProvider {
public:
    virtual ~IEmbeddingProvider() = default;

    /**
     * @brief 生成单个文本的嵌入向量
     * @param text 输入文本
     * @return 嵌入向量或错误信息
     */
    virtual DearTs::Core::Result<EmbeddingVector, std::string> generate_embedding(
        const std::string& text
    ) = 0;

    /**
     * @brief 批量生成嵌入向量
     * @param texts 输入文本列表
     * @return 嵌入向量列表或错误信息
     */
    virtual DearTs::Core::Result<std::vector<EmbeddingVector>, std::string> generate_embeddings_batch(
        const std::vector<std::string>& texts
    ) = 0;

    /**
     * @brief 获取模型名称
     */
    virtual std::string get_model_name() const = 0;

    /**
     * @brief 检查提供者是否可用
     */
    virtual bool is_available() const = 0;
};

/**
 * @brief Ollama 嵌入提供者实现
 *
 * 使用 Ollama 本地服务生成嵌入向量
 */
class OllamaEmbeddingProvider : public IEmbeddingProvider {
public:
    /**
     * @brief 构造函数
     * @param base_url Ollama 服务地址
     * @param model 嵌入模型名称（默认: nomic-embed-text）
     */
    OllamaEmbeddingProvider(
        const std::string& base_url = "http://localhost:11434",
        const std::string& model = "nomic-embed-text"
    );

    ~OllamaEmbeddingProvider() override = default;

    // IEmbeddingProvider 接口实现
    DearTs::Core::Result<EmbeddingVector, std::string> generate_embedding(
        const std::string& text
    ) override;

    DearTs::Core::Result<std::vector<EmbeddingVector>, std::string> generate_embeddings_batch(
        const std::vector<std::string>& texts
    ) override;

    std::string get_model_name() const override { return m_model; }
    bool is_available() const override { return test_connection(); }

private:
    /**
     * @brief 测试 Ollama 连接
     */
    bool test_connection() const;

    std::string m_base_url;
    std::string m_model;
};

/**
 * @brief 嵌入提供者工厂
 *
 * 创建不同类型的嵌入提供者
 */
class EmbeddingProviderFactory {
public:
    /**
     * @brief 创建 Ollama 嵌入提供者
     * @param base_url Ollama 服务地址
     * @param model 嵌入模型名称
     * @return 嵌入提供者智能指针
     */
    static std::unique_ptr<IEmbeddingProvider> create_ollama_provider(
        const std::string& base_url = "http://localhost:11434",
        const std::string& model = "nomic-embed-text"
    );

    /**
     * @brief 创建 HTTP 嵌入提供者（LM Studio、vLLM、TGI 等）
     * @param base_url API 服务地址 (如 http://localhost:1234/v1)
     * @param model 嵌入模型名称 (如 text-embedding-nomic-embed-text-v1.5)
     * @param api_key API 密钥（可选）
     * @return 嵌入提供者智能指针
     */
    static std::unique_ptr<IEmbeddingProvider> create_http_provider(
        const std::string& base_url = "http://localhost:1234/v1",
        const std::string& model = "text-embedding-nomic-embed-text-v1.5",
        const std::string& api_key = ""
    );

    /**
     * @brief 创建自定义嵌入提供者（使用回调函数）
     * @param generate_func 嵌入生成函数
     * @param model_name 模型名称
     * @return 嵌入提供者智能指针
     */
    static std::unique_ptr<IEmbeddingProvider> create_custom_provider(
        std::function<DearTs::Core::Result<EmbeddingVector, std::string>(const std::string&)> generate_func,
        const std::string& model_name = "custom"
    );
};

/**
 * @brief 自定义嵌入提供者实现
 *
 * 使用用户提供的回调函数生成嵌入
 */
class CustomEmbeddingProvider : public IEmbeddingProvider {
public:
    using GenerateFunction = std::function<DearTs::Core::Result<EmbeddingVector, std::string>(const std::string&)>;

    CustomEmbeddingProvider(
        GenerateFunction generate_func,
        const std::string& model_name
    ) : m_generate_func(std::move(generate_func)),
        m_model_name(model_name) {}

    DearTs::Core::Result<EmbeddingVector, std::string> generate_embedding(
        const std::string& text
    ) override {
        return m_generate_func(text);
    }

    DearTs::Core::Result<std::vector<EmbeddingVector>, std::string> generate_embeddings_batch(
        const std::vector<std::string>& texts
    ) override {
        std::vector<EmbeddingVector> results;
        results.reserve(texts.size());

        for (const auto& text : texts) {
            auto result = generate_embedding(text);
            if (result.isErr()) {
                return DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>::err(result.error());
            }
            results.push_back(result.unwrap());
        }

        return DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>::ok(results);
    }

    std::string get_model_name() const override { return m_model_name; }
    bool is_available() const override { return m_generate_func != nullptr; }

private:
    GenerateFunction m_generate_func;
    std::string m_model_name;
};

/**
 * @brief HTTP 嵌入提供者实现
 *
 * 使用 OpenAI 兼容 API（LM Studio、vLLM、TGI 等）生成嵌入向量
 */
class HTTPEmbeddingProvider : public IEmbeddingProvider {
public:
    /**
     * @brief 构造函数
     * @param base_url API 服务地址 (如 http://localhost:1234/v1)
     * @param model 嵌入模型名称 (如 text-embedding-nomic-embed-text-v1.5)
     * @param api_key API 密钥（可选，本地服务通常不需要）
     */
    HTTPEmbeddingProvider(
        const std::string& base_url = "http://localhost:1234/v1",
        const std::string& model = "text-embedding-nomic-embed-text-v1.5",
        const std::string& api_key = ""
    );

    ~HTTPEmbeddingProvider() override = default;

    // IEmbeddingProvider 接口实现
    DearTs::Core::Result<EmbeddingVector, std::string> generate_embedding(
        const std::string& text
    ) override;

    DearTs::Core::Result<std::vector<EmbeddingVector>, std::string> generate_embeddings_batch(
        const std::vector<std::string>& texts
    ) override;

    std::string get_model_name() const override { return m_model; }
    bool is_available() const override { return test_connection(); }

private:
    /**
     * @brief 测试 API 连接
     */
    bool test_connection() const;

    /**
     * @brief 发送 HTTP POST 请求
     */
    DearTs::Core::Result<std::string, std::string> send_request(
        const std::string& endpoint,
        const std::string& json_body
    ) const;

    std::string m_base_url;
    std::string m_model;
    std::string m_api_key;
};

} // namespace DearTs::Plugins::MemoryCore::RAG
