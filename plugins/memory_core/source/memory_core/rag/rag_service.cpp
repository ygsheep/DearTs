/**
 * @file rag_service.cpp
 * @brief RAG 服务实现
 */

#include "memory_core/rag/rag_service.hpp"
#include "memory_core/rag/embedding_cache.hpp"
#include "memory_core/memory/memory_manager.hpp"
#include "memory_core/persistence/database.hpp"
#include "liblogger/logger.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <set>

namespace DearTs::Plugins::MemoryCore::RAG {

// ============ 单例实现 ============

RAGService& RAGService::instance() {
    static RAGService instance;
    return instance;
}

// ============ 初始化 ============

void RAGService::initialize(const CacheConfig& embedding_cache_config) {
    if (m_initialized) {
        LOG_WARN("RAGService already initialized");
        return;
    }

    // 初始化嵌入缓存
    EmbeddingCache::instance().initialize(embedding_cache_config);

    m_initialized = true;
    m_default_model = "nomic-embed-text";

    LOG_INFO("RAGService initialized");
}

void RAGService::shutdown() {
    if (!m_initialized) {
        return;
    }

    // 打印统计信息
    log_stats();

    m_initialized = false;
    LOG_INFO("RAGService shutdown");
}

// ============ 语义搜索 ============

DearTs::Core::Result<std::vector<RAGResult>, std::string>
RAGService::query(const std::string& query, const RAGQueryOptions& options) {
    if (!m_initialized) {
        return DearTs::Core::Result<std::vector<RAGResult>, std::string>::err(
            "RAGService not initialized"
        );
    }

    LOG_INFO("RAG query: '{}', max_results={}, enable_hybrid={}",
             query, options.max_results, options.enable_hybrid_search);

    // TODO: 阶段 4.2 实现完整的 RAG 查询
    // 当前占位符实现

    std::vector<RAGResult> results;

    // 1. 生成查询嵌入
    // auto embedding_result = generate_embedding(query, options.embedding_model);
    // if (embedding_result.isErr()) {
    //     return DearTs::Core::Result<std::vector<RAGResult>, std::string>::err(
    //         "Failed to generate query embedding: " + embedding_result.error()
    //     );
    // }
    // auto& query_embedding = embedding_result.unwrap().vector;

    // 2. 执行搜索
    // if (options.enable_hybrid_search) {
    //     results = hybrid_search(query, query_embedding, options);
    // } else {
    //     results = semantic_search(query_embedding, options);
    // }

    // 3. 过滤和排序
    // results.erase(
    //     std::remove_if(results.begin(), results.end(),
    //         [&](const RAGResult& r) {
    //             return r.combined_score < options.min_similarity;
    //         }),
    //     results.end()
    // );

    // // 限制结果数量
    // if (results.size() > static_cast<size_t>(options.max_results)) {
    //     results.resize(options.max_results);
    // }

    LOG_INFO("RAG query completed: {} results", results.size());
    return DearTs::Core::Result<std::vector<RAGResult>, std::string>::ok(results);
}

DearTs::Core::Result<std::vector<std::vector<RAGResult>>, std::string>
RAGService::query_batch(
    const std::vector<std::string>& queries,
    const RAGQueryOptions& options
) {
    std::vector<std::vector<RAGResult>> all_results;

    for (const auto& query_text : queries) {
        auto result = query(query_text, options);
        if (result.isOk()) {
            all_results.push_back(result.unwrap());
        } else {
            return DearTs::Core::Result<std::vector<std::vector<RAGResult>>, std::string>::err(result.error());
        }
    }

    LOG_INFO("RAG batch query completed: {} queries", queries.size());
    return DearTs::Core::Result<std::vector<std::vector<RAGResult>>, std::string>::ok(all_results);
}

// ============ 上下文构建 ============

DearTs::Core::Result<std::string, std::string>
RAGService::build_context(
    const std::string& query_text,
    int max_context_length,
    const RAGQueryOptions& options
) {
    // 执行查询
    auto query_result = query(query_text, options);
    if (query_result.isErr()) {
        return DearTs::Core::Result<std::string, std::string>::err(query_result.error());
    }

    auto results = query_result.unwrap();

    // 构建上下文
    std::ostringstream context;
    context << "相关记忆：\n\n";

    int current_length = static_cast<int>(context.str().length());

    for (const auto& result : results) {
        std::string memory_text = format_memory_as_context(result.memory);

        // 检查长度限制
        if (current_length + static_cast<int>(memory_text.length()) > max_context_length) {
            break;
        }

        context << memory_text << "\n\n";
        current_length += static_cast<int>(memory_text.length()) + 2;
    }

    LOG_INFO("Built context: {} characters, {} memories", current_length, results.size());
    return DearTs::Core::Result<std::string, std::string>::ok(context.str());
}

DearTs::Core::Result<std::string, std::string>
RAGService::build_prompt(
    const std::string& user_query,
    const std::string& system_prompt,
    int max_context_length
) {
    // 构建上下文
    auto context_result = build_context(user_query, max_context_length);
    if (context_result.isErr()) {
        return DearTs::Core::Result<std::string, std::string>::err(context_result.error());
    }

    std::ostringstream prompt;
    prompt << system_prompt << "\n\n";
    prompt << context_result.unwrap() << "\n";
    prompt << "用户问题：" << user_query << "\n";
    prompt << "请根据以上相关记忆回答用户问题。";

    return DearTs::Core::Result<std::string, std::string>::ok(prompt.str());
}

// ============ 向量嵌入 ============

DearTs::Core::Result<Embedding, std::string>
RAGService::generate_embedding(const std::string& text, const std::string& model) {
    // TODO: 阶段 4.3 集成 Ollama embed API
    // 当前占位符实现

    // 检查缓存
    auto& cache = EmbeddingCache::instance();
    if (cache.contains(text)) {
        auto cached = cache.get(text);
        if (cached.has_value()) {
            LOG_DEBUG("Embedding cache hit for: {}", text.substr(0, 50));
            return DearTs::Core::Result<Embedding, std::string>::ok(cached.value());
        }
    }

    // 占位符：返回零向量
    Embedding embedding;
    embedding.model = model;
    embedding.dimension = 768;  // 常见维度
    embedding.vector = std::vector<float>(embedding.dimension, 0.0f);
    embedding.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    embedding.accessed_at = embedding.created_at;

    // 保存到缓存
    cache.put(text, embedding);

    LOG_DEBUG("Generated placeholder embedding for: {}", text.substr(0, 50));
    return DearTs::Core::Result<Embedding, std::string>::ok(embedding);
}

DearTs::Core::Result<std::vector<Embedding>, std::string>
RAGService::generate_embeddings_batch(
    const std::vector<std::string>& texts,
    const std::string& model
) {
    std::vector<Embedding> embeddings;

    for (const auto& text : texts) {
        auto result = generate_embedding(text, model);
        if (result.isOk()) {
            embeddings.push_back(result.unwrap());
        } else {
            return DearTs::Core::Result<std::vector<Embedding>, std::string>::err(result.error());
        }
    }

    LOG_INFO("Generated {} embeddings", embeddings.size());
    return DearTs::Core::Result<std::vector<Embedding>, std::string>::ok(embeddings);
}

// ============ 相似度计算 ============

double RAGService::cosine_similarity(const std::vector<float>& vec1, const std::vector<float>& vec2) {
    if (vec1.size() != vec2.size()) {
        return 0.0;
    }

    double dot_product = 0.0;
    double norm1 = 0.0;
    double norm2 = 0.0;

    for (size_t i = 0; i < vec1.size(); ++i) {
        dot_product += vec1[i] * vec2[i];
        norm1 += vec1[i] * vec1[i];
        norm2 += vec2[i] * vec2[i];
    }

    norm1 = std::sqrt(norm1);
    norm2 = std::sqrt(norm2);

    if (norm1 < 1e-10 || norm2 < 1e-10) {
        return 0.0;
    }

    return dot_product / (norm1 * norm2);
}

double RAGService::keyword_match_score(const std::string& query, const std::string& content) {
    if (query.empty() || content.empty()) {
        return 0.0;
    }

    // 简单的关键词匹配：计算查询词在内容中出现的比例
    std::set<std::string> query_words;
    std::istringstream query_stream(query);
    std::string word;

    while (query_stream >> word) {
        // 转换为小写
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        query_words.insert(word);
    }

    if (query_words.empty()) {
        return 0.0;
    }

    // 统计匹配数
    size_t matches = 0;
    std::string lower_content = content;
    std::transform(lower_content.begin(), lower_content.end(), lower_content.begin(), ::tolower);

    for (const auto& query_word : query_words) {
        if (lower_content.find(query_word) != std::string::npos) {
            matches++;
        }
    }

    return static_cast<double>(matches) / query_words.size();
}

// ============ 统计信息 ============

CacheStats RAGService::get_cache_stats() const {
    return EmbeddingCache::instance().get_stats();
}

void RAGService::log_stats() const {
    EmbeddingCache::instance().log_stats();
}

// ============ 私有辅助方法 ============

DearTs::Core::Result<Embedding, std::string>
RAGService::get_memory_embedding(int64_t memory_id) {
    // TODO: 从数据库获取记忆的嵌入
    Embedding embedding;
    return DearTs::Core::Result<Embedding, std::string>::ok(embedding);
}

std::vector<RAGResult> RAGService::semantic_search(
    const std::vector<float>& query_embedding,
    const RAGQueryOptions& options
) {
    // TODO: 实现语义搜索
    std::vector<RAGResult> results;
    return results;
}

std::vector<RAGResult> RAGService::keyword_search(
    const std::string& query,
    const RAGQueryOptions& options
) {
    // TODO: 实现关键词搜索
    std::vector<RAGResult> results;
    return results;
}

std::vector<RAGResult> RAGService::hybrid_search(
    const std::string& query,
    const std::vector<float>& query_embedding,
    const RAGQueryOptions& options
) {
    // 执行语义搜索和关键词搜索
    auto semantic_results = semantic_search(query_embedding, options);
    auto keyword_results = keyword_search(query, options);

    // 合并和排序
    return merge_and_rank_results(semantic_results, keyword_results, options);
}

std::vector<RAGResult> RAGService::merge_and_rank_results(
    std::vector<RAGResult> semantic_results,
    std::vector<RAGResult> keyword_results,
    const RAGQueryOptions& options
) {
    // TODO: 实现结果合并和排序
    // 当前简单返回语义搜索结果
    return semantic_results;
}

std::string RAGService::format_memory_as_context(const Memory::Memory& memory) {
    std::ostringstream oss;

    // 添加类型标签
    oss << "[" << Memory::Memory::type_to_string(memory.type) << "] ";

    // 添加内容
    oss << memory.content;

    // 添加元数据
    if (memory.importance > 0.7) {
        oss << " (重要性: " << static_cast<int>(memory.importance * 100) << "%)";
    }

    return oss.str();
}

} // namespace DearTs::Plugins::MemoryCore::RAG
