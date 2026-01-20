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
#include <format>

// SQLite3 头文件 - 条件编译
#ifdef SQLITE3_FOUND
    #include <sqlite3.h>
#endif

namespace DearTs::Plugins::MemoryCore::RAG {

// ============ 单例实现 ============

RAGService& RAGService::instance() {
    static RAGService instance;
    return instance;
}

// ============ 初始化 ============

void RAGService::initialize(
    const CacheConfig& embedding_cache_config,
    std::unique_ptr<IEmbeddingProvider> embedding_provider
) {
    if (m_initialized) {
        LOG_WARN("RAGService already initialized");
        return;
    }

    // 初始化嵌入缓存
    EmbeddingCache::instance().initialize(embedding_cache_config);

    // 设置嵌入提供者
    if (embedding_provider) {
        m_embedding_provider = std::move(embedding_provider);
        m_default_model = m_embedding_provider->get_model_name();
        LOG_INFO("RAGService: Using embedding provider: {}", m_default_model);
    } else {
        m_default_model = "nomic-embed-text";
        LOG_WARN("RAGService: No embedding provider provided, semantic search disabled");
    }

    m_initialized = true;

    LOG_INFO("RAGService initialized");
}

void RAGService::set_embedding_provider(std::unique_ptr<IEmbeddingProvider> provider) {
    m_embedding_provider = std::move(provider);
    if (m_embedding_provider) {
        m_default_model = m_embedding_provider->get_model_name();
        LOG_INFO("RAGService: Embedding provider set to: {}", m_default_model);
    }
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

    std::vector<RAGResult> results;

    // 1. 生成查询嵌入
    auto embedding_result = generate_embedding(query, options.embedding_model);
    if (embedding_result.isErr()) {
        // 如果嵌入生成失败，回退到关键词搜索
        LOG_WARN("Failed to generate query embedding: {}, falling back to keyword search",
                 embedding_result.error());
        results = keyword_search(query, options);
    } else {
        auto& query_embedding = embedding_result.unwrap().vector;

        // 2. 执行搜索
        if (options.enable_hybrid_search && m_embedding_provider) {
            results = hybrid_search(query, query_embedding, options);
        } else {
            results = semantic_search(query_embedding, options);
        }
    }

    // 3. 过滤和排序
    results.erase(
        std::remove_if(results.begin(), results.end(),
            [&](const RAGResult& r) {
                return r.combined_score < options.min_similarity;
            }),
        results.end()
    );

    // 限制结果数量
    if (results.size() > static_cast<size_t>(options.max_results)) {
        results.resize(options.max_results);
    }

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
    // 检查缓存
    auto& cache = EmbeddingCache::instance();
    if (cache.contains(text)) {
        auto cached = cache.get(text);
        if (cached.has_value()) {
            LOG_DEBUG("Embedding cache hit for: {}", text.substr(0, 50));
            return DearTs::Core::Result<Embedding, std::string>::ok(cached.value());
        }
    }

    // 检查嵌入提供者
    if (!m_embedding_provider) {
        return DearTs::Core::Result<Embedding, std::string>::err(
            "No embedding provider available"
        );
    }

    // 使用嵌入提供者生成向量
    auto result = m_embedding_provider->generate_embedding(text);
    if (result.isErr()) {
        return DearTs::Core::Result<Embedding, std::string>::err(result.error());
    }

    auto& embedding_vector = result.unwrap();

    // 转换为 Embedding 结构
    Embedding embedding;
    embedding.model = embedding_vector.model;
    embedding.dimension = embedding_vector.dimension;
    embedding.vector = embedding_vector.data;
    embedding.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    embedding.accessed_at = embedding.created_at;

    // 保存到缓存
    cache.put(text, embedding);

    LOG_DEBUG("Generated embedding for: {}, dimension={}, duration={} ms",
              text.substr(0, 50), embedding.dimension, embedding_vector.duration_ms);

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
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<Embedding, std::string>::err("Database not initialized");
    }

    // 获取原始数据库指针
    sqlite3* sqlite_db = db.get_db();
    if (!sqlite_db) {
        return DearTs::Core::Result<Embedding, std::string>::err("Invalid database handle");
    }

    // 查询记忆的 embedding_id
    const char* sql = R"(
        SELECT embedding_id FROM memories WHERE id = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(sqlite_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<Embedding, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(sqlite_db))
        );
    }

    sqlite3_bind_int64(stmt, 1, memory_id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return DearTs::Core::Result<Embedding, std::string>::err(
            std::format("Memory not found: {}", memory_id)
        );
    }

    // 获取 embedding_id（可能为 NULL）
    int64_t embedding_id = sqlite3_column_type(stmt, 0) != SQLITE_NULL ?
        sqlite3_column_int64(stmt, 0) : 0;

    sqlite3_finalize(stmt);

    if (!embedding_id) {
        return DearTs::Core::Result<Embedding, std::string>::err(
            std::format("Memory {} has no embedding", memory_id)
        );
    }

    // 从数据库获取嵌入向量
    auto embedding_record = db.get_embedding(embedding_id);
    if (embedding_record.isErr()) {
        return DearTs::Core::Result<Embedding, std::string>::err(embedding_record.error());
    }

    auto& record = embedding_record.unwrap();

    // 解析 JSON 格式的向量数据
    auto parse_result = EmbeddingVector::from_json(record.vector_data);
    if (parse_result.isErr()) {
        return DearTs::Core::Result<Embedding, std::string>::err(parse_result.error());
    }

    auto& vector = parse_result.unwrap();

    // 转换为 Embedding 结构
    Embedding embedding;
    embedding.model = record.model;
    embedding.dimension = record.dimension;
    embedding.vector = vector.data;
    embedding.created_at = record.created_at;
    embedding.accessed_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    return DearTs::Core::Result<Embedding, std::string>::ok(embedding);
#else
    return DearTs::Core::Result<Embedding, std::string>::err("SQLite3 not available");
#endif
}

std::vector<RAGResult> RAGService::semantic_search(
    const std::vector<float>& query_embedding,
    const RAGQueryOptions& options
) {
    std::vector<RAGResult> results;

#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        LOG_ERROR("RAG semantic search: Database not initialized");
        return results;
    }

    // 获取原始数据库指针
    sqlite3* sqlite_db = db.get_db();
    if (!sqlite_db) {
        LOG_ERROR("RAG semantic search: Invalid database handle");
        return results;
    }

    // 查询所有有嵌入向量的记忆
    const char* sql = R"(
        SELECT id, type, content, source_conversation_id,
               importance, created_at, accessed_count, embedding_id
        FROM memories
        WHERE embedding_id IS NOT NULL
        ORDER BY importance DESC, accessed_count DESC
        LIMIT ?
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(sqlite_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        LOG_ERROR("RAG semantic search: Failed to prepare statement: {}", sqlite3_errmsg(sqlite_db));
        return results;
    }

    // 绑定参数（获取更多结果用于相似度计算和过滤）
    sqlite3_bind_int(stmt, 1, options.max_results * 3);

    // 执行查询并收集结果
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        RAGResult result;
        result.memory.id = sqlite3_column_int64(stmt, 0);

        const char* type_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.memory.type = Memory::Memory::string_to_type(type_str ? type_str : "fact");

        const char* content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.memory.content = content ? content : "";

        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
            const char* conv_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            result.memory.source_conversation_id = std::string(conv_id ? conv_id : "");
        }

        result.memory.importance = sqlite3_column_double(stmt, 4);
        result.memory.created_at = sqlite3_column_int64(stmt, 5);
        result.memory.accessed_count = sqlite3_column_int(stmt, 6);

        // 获取嵌入向量并计算余弦相似度
        int64_t embedding_id = sqlite3_column_int64(stmt, 7);
        if (embedding_id > 0) {
            // 尝试从数据库获取嵌入
            auto embedding_result = db.get_embedding(embedding_id);
            if (embedding_result.isOk()) {
                auto& record = embedding_result.unwrap();
                auto parse_result = EmbeddingVector::from_json(record.vector_data);

                if (parse_result.isOk()) {
                    auto& memory_embedding = parse_result.unwrap();
                    result.similarity = cosine_similarity(query_embedding, memory_embedding.data);
                }
            }
        }

        // 设置关键词分数为 0（纯语义搜索）
        result.keyword_score = 0.0;
        result.combined_score = result.similarity;

        results.push_back(result);
    }

    sqlite3_finalize(stmt);

    // 按相似度排序
    std::sort(results.begin(), results.end(),
        [](const RAGResult& a, const RAGResult& b) {
            return a.similarity > b.similarity;
        });

    LOG_INFO("RAG semantic search: {} candidates found", results.size());
#else
    LOG_WARN("RAG semantic search called but SQLite3 not available");
#endif

    return results;
}

std::vector<RAGResult> RAGService::keyword_search(
    const std::string& query,
    const RAGQueryOptions& options
) {
    std::vector<RAGResult> results;

#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        LOG_ERROR("RAG keyword search: Database not initialized");
        return results;
    }

    // 获取原始数据库指针
    sqlite3* sqlite_db = db.get_db();
    if (!sqlite_db) {
        LOG_ERROR("RAG keyword search: Invalid database handle");
        return results;
    }

    // 构建查询 SQL
    const char* sql = R"(
        SELECT id, type, content, source_conversation_id, importance, created_at, accessed_count
        FROM memories
        WHERE content LIKE ? OR source_conversation_id LIKE ?
        ORDER BY importance DESC, accessed_count DESC
        LIMIT ?
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(sqlite_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        LOG_ERROR("RAG keyword search: Failed to prepare statement: {}", sqlite3_errmsg(sqlite_db));
        return results;
    }

    // 绑定参数（使用 %query% 进行模糊匹配）
    std::string pattern = "%" + query + "%";
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, options.max_results * 2);  // 获取更多结果用于过滤

    // 执行查询并收集结果
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        RAGResult result;
        result.memory.id = sqlite3_column_int64(stmt, 0);

        const char* type_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.memory.type = Memory::Memory::string_to_type(type_str ? type_str : "fact");

        const char* content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.memory.content = content ? content : "";

        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
            const char* conv_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            result.memory.source_conversation_id = std::string(conv_id ? conv_id : "");
        }

        result.memory.importance = sqlite3_column_double(stmt, 4);
        result.memory.created_at = sqlite3_column_int64(stmt, 5);
        result.memory.accessed_count = sqlite3_column_int(stmt, 6);

        // 计算关键词匹配分数
        result.keyword_score = keyword_match_score(query, result.memory.content);

        // 组合分数（简化版本：70% 关键词 + 30% 重要性）
        result.combined_score = result.keyword_score * 0.7 + result.memory.importance * 0.3;

        results.push_back(result);
    }

    sqlite3_finalize(stmt);

    // 过滤低分结果
    results.erase(
        std::remove_if(results.begin(), results.end(),
            [&](const RAGResult& r) {
                return r.combined_score < options.min_similarity;
            }),
        results.end()
    );

    // 按组合分数排序
    std::sort(results.begin(), results.end(),
        [](const RAGResult& a, const RAGResult& b) {
            return a.combined_score > b.combined_score;
        });

    // 限制结果数量
    if (results.size() > static_cast<size_t>(options.max_results)) {
        results.resize(options.max_results);
    }

    LOG_INFO("RAG keyword search: {} results for query '{}'", results.size(), query);
#else
    LOG_WARN("RAG keyword search called but SQLite3 not available");
#endif

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
    // 使用 std::unordered_map 合并结果（按记忆 ID 去重）
    std::unordered_map<int64_t, RAGResult> merged_map;

    // 添加语义搜索结果
    for (auto& result : semantic_results) {
        result.calculate_combined_score(0.7, 0.3);  // 70% 语义 + 30% 关键词
        merged_map[result.memory.id] = std::move(result);
    }

    // 合并关键词搜索结果
    for (auto& result : keyword_results) {
        auto it = merged_map.find(result.memory.id);
        if (it != merged_map.end()) {
            // 已存在，更新分数（取平均值）
            RAGResult& existing = it->second;
            existing.keyword_score = result.keyword_score;
            existing.calculate_combined_score(0.7, 0.3);
        } else {
            // 不存在，添加新结果
            result.similarity = 0.0;  // 关键词搜索没有语义分数
            result.calculate_combined_score(0.7, 0.3);
            merged_map[result.memory.id] = std::move(result);
        }
    }

    // 转换为 vector 并按组合分数排序
    std::vector<RAGResult> results;
    results.reserve(merged_map.size());
    for (auto& [id, result] : merged_map) {
        results.push_back(std::move(result));
    }

    std::sort(results.begin(), results.end(),
        [](const RAGResult& a, const RAGResult& b) {
            return a.combined_score > b.combined_score;
        });

    LOG_DEBUG("RAG merged results: {} total ({} semantic, {} keyword)",
              results.size(), semantic_results.size(), keyword_results.size());

    return results;
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
