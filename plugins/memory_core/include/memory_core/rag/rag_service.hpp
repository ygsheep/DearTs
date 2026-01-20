/**
 * @file rag_service.hpp
 * @brief RAG 服务 - 检索增强生成服务
 *
 * 功能：
 * - 语义搜索（基于向量相似度）
 * - 混合搜索（语义 + 关键词）
 * - 上下文构建
 * - 查询优化
 */

#pragma once

#include "core/result.h"
#include "memory_core/rag/embedding_cache.hpp"
#include "memory_core/rag/embedding_provider.hpp"
#include "memory_core/memory/memory_manager.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace DearTs::Plugins::MemoryCore::RAG {

/**
 * @brief RAG 查询结果项
 */
struct RAGResult {
    Memory::Memory memory;              ///< 相关记忆
    double similarity;                  ///< 语义相似度 [0-1]
    double keyword_score;               ///< 关键词匹配分数 [0-1]
    double combined_score;              ///< 综合分数 [0-1]

    /**
     * @brief 计算综合分数（语义和关键词的加权组合）
     * @param semantic_weight 语义权重
     * @param keyword_weight 关键词权重
     */
    void calculate_combined_score(double semantic_weight = 0.7, double keyword_weight = 0.3) {
        combined_score = semantic_weight * similarity + keyword_weight * keyword_score;
    }
};

/**
 * @brief RAG 查询选项
 */
struct RAGQueryOptions {
    int max_results;                    ///< 最大结果数
    double min_similarity;              ///< 最小语义相似度 [0-1]
    double min_keyword_score;           ///< 最小关键词分数 [0-1]
    bool use_cache;                     ///< 是否使用缓存
    bool enable_hybrid_search;          ///< 是否启用混合搜索
    std::string embedding_model;        ///< 嵌入模型名称
    std::vector<Memory::MemoryType> memory_types;  ///< 限制搜索的记忆类型

    /**
     * @brief 默认选项
     */
    static RAGQueryOptions default_options() {
        return RAGQueryOptions{
            .max_results = 5,
            .min_similarity = 0.5,
            .min_keyword_score = 0.3,
            .use_cache = true,
            .enable_hybrid_search = true,
            .embedding_model = "nomic-embed-text",
            .memory_types = {}
        };
    }
};

/**
 * @brief RAG 服务类
 *
 * 提供语义搜索和上下文构建功能
 */
class RAGService {
public:
    /**
     * @brief 获取单例实例
     */
    static RAGService& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    RAGService(const RAGService&) = delete;
    RAGService& operator=(const RAGService&) = delete;

    // ============ 初始化 ============

    /**
     * @brief 初始化 RAG 服务
     * @param embedding_cache_config 嵌入缓存配置
     * @param embedding_provider 嵌入向量提供者（可选）
     */
    void initialize(
        const CacheConfig& embedding_cache_config = CacheConfig::default_config(),
        std::unique_ptr<IEmbeddingProvider> embedding_provider = nullptr
    );

    /**
     * @brief 设置嵌入向量提供者
     * @param provider 嵌入向量提供者
     */
    void set_embedding_provider(std::unique_ptr<IEmbeddingProvider> provider);

    /**
     * @brief 关闭 RAG 服务
     */
    void shutdown();

    // ============ 语义搜索 ============

    /**
     * @brief 执行 RAG 查询
     * @param query 查询文本
     * @param options 查询选项
     * @return 查询结果列表或错误信息
     */
    DearTs::Core::Result<std::vector<RAGResult>, std::string> query(
        const std::string& query,
        const RAGQueryOptions& options = RAGQueryOptions::default_options()
    );

    /**
     * @brief 批量查询
     * @param queries 查询列表
     * @param options 查询选项
     * @return 每个查询的结果列表
     */
    DearTs::Core::Result<std::vector<std::vector<RAGResult>>, std::string> query_batch(
        const std::vector<std::string>& queries,
        const RAGQueryOptions& options = RAGQueryOptions::default_options()
    );

    // ============ 上下文构建 ============

    /**
     * @brief 为查询构建上下文
     * @param query 查询文本
     * @param max_context_length 最大上下文长度（字符）
     * @param options 查询选项
     * @return 构建的上下文字符串或错误信息
     */
    DearTs::Core::Result<std::string, std::string> build_context(
        const std::string& query,
        int max_context_length = 2000,
        const RAGQueryOptions& options = RAGQueryOptions::default_options()
    );

    /**
     * @brief 构建增强的提示词
     * @param user_query 用户查询
     * @param system_prompt 系统提示词
     * @param max_context_length 最大上下文长度
     * @return 增强后的提示词或错误信息
     */
    DearTs::Core::Result<std::string, std::string> build_prompt(
        const std::string& user_query,
        const std::string& system_prompt,
        int max_context_length = 2000
    );

    // ============ 向量嵌入 ============

    /**
     * @brief 生成文本的向量嵌入
     * @param text 输入文本
     * @param model 模型名称
     * @return 向量嵌入或错误信息
     */
    DearTs::Core::Result<Embedding, std::string> generate_embedding(
        const std::string& text,
        const std::string& model = "nomic-embed-text"
    );

    /**
     * @brief 批量生成向量嵌入
     * @param texts 文本列表
     * @param model 模型名称
     * @return 向量嵌入列表或错误信息
     */
    DearTs::Core::Result<std::vector<Embedding>, std::string> generate_embeddings_batch(
        const std::vector<std::string>& texts,
        const std::string& model = "nomic-embed-text"
    );

    // ============ 相似度计算 ============

    /**
     * @brief 计算两个向量的余弦相似度
     * @param vec1 向量 1
     * @param vec2 向量 2
     * @return 相似度 [0-1]
     */
    static double cosine_similarity(const std::vector<float>& vec1, const std::vector<float>& vec2);

    /**
     * @brief 计算关键词匹配分数
     * @param query 查询文本
     * @param content 内容文本
     * @return 匹配分数 [0-1]
     */
    static double keyword_match_score(const std::string& query, const std::string& content);

    // ============ 统计信息 ============

    /**
     * @brief 获取缓存统计信息
     */
    CacheStats get_cache_stats() const;

    /**
     * @brief 打印统计信息
     */
    void log_stats() const;

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    RAGService() = default;

    /**
     * @brief 析构函数
     */
    ~RAGService() = default;

    /**
     * @brief 从数据库获取记忆的嵌入
     * @param memory_id 记忆 ID
     * @return 向量嵌入
     */
    DearTs::Core::Result<Embedding, std::string> get_memory_embedding(int64_t memory_id);

    /**
     * @brief 语义搜索（仅向量相似度）
     */
    std::vector<RAGResult> semantic_search(
        const std::vector<float>& query_embedding,
        const RAGQueryOptions& options
    );

    /**
     * @brief 关键词搜索
     */
    std::vector<RAGResult> keyword_search(
        const std::string& query,
        const RAGQueryOptions& options
    );

    /**
     * @brief 混合搜索（语义 + 关键词）
     */
    std::vector<RAGResult> hybrid_search(
        const std::string& query,
        const std::vector<float>& query_embedding,
        const RAGQueryOptions& options
    );

    /**
     * @brief 合并和排序搜索结果
     */
    std::vector<RAGResult> merge_and_rank_results(
        std::vector<RAGResult> semantic_results,
        std::vector<RAGResult> keyword_results,
        const RAGQueryOptions& options
    );

    /**
     * @brief 格式化记忆为上下文字符串
     */
    std::string format_memory_as_context(const Memory::Memory& memory);

    // ============ 成员变量 ============

    bool m_initialized = false;          ///< 是否已初始化
    std::string m_default_model;         ///< 默认嵌入模型
    std::unique_ptr<IEmbeddingProvider> m_embedding_provider;  ///< 嵌入向量提供者
};

} // namespace DearTs::Plugins::MemoryCore::RAG
