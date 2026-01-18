/**
 * @file embedding_cache.cpp
 * @brief 向量嵌入缓存实现
 */

#include "memory_core/rag/embedding_cache.hpp"
#include "liblogger/logger.h"
#include <chrono>

namespace DearTs::Plugins::MemoryCore::RAG {

// ============ 单例实现 ============

EmbeddingCache& EmbeddingCache::instance() {
    static EmbeddingCache instance;
    return instance;
}

// ============ 初始化 ============

void EmbeddingCache::initialize(const CacheConfig& config) {
    std::lock_guard lock(m_mutex);

    m_config = config;
    m_current_size_bytes = 0;
    m_stats = CacheStats{};

    LOG_INFO("EmbeddingCache initialized: max_entries={}, max_size_mb={}",
             m_config.max_entries, m_config.max_size_mb);
}

void EmbeddingCache::clear() {
    std::lock_guard lock(m_mutex);

    size_t count = m_cache_map.size();
    m_cache_map.clear();
    m_lru_list.clear();
    m_current_size_bytes = 0;

    LOG_INFO("EmbeddingCache cleared: removed {} entries", count);
}

// ============ 缓存操作 ============

std::optional<Embedding> EmbeddingCache::get(const std::string& text) {
    std::lock_guard lock(m_mutex);

    auto it = m_cache_map.find(text);
    if (it == m_cache_map.end()) {
        // 缓存未命中
        if (m_config.enable_stats) {
            m_stats.misses++;
        }
        return std::nullopt;
    }

    // 缓存命中
    if (m_config.enable_stats) {
        m_stats.hits++;
    }

    // 更新 LRU 顺序：将访问的条目移到列表前端
    m_lru_list.erase(it->second.first);
    m_lru_list.push_front(text);
    it->second.first = m_lru_list.begin();

    // 更新访问时间
    it->second.second.accessed_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    LOG_DEBUG("EmbeddingCache hit: text='{}'", text.substr(0, 50));
    return it->second.second;
}

void EmbeddingCache::put(const std::string& text, const Embedding& embedding) {
    std::lock_guard lock(m_mutex);

    // 检查是否已存在
    auto it = m_cache_map.find(text);
    if (it != m_cache_map.end()) {
        // 更新现有条目
        m_current_size_bytes -= it->second.second.size_bytes();
        m_lru_list.erase(it->second.first);
        m_cache_map.erase(it);
    }

    // 添加到列表前端
    m_lru_list.push_front(text);
    m_cache_map[text] = std::make_pair(m_lru_list.begin(), embedding);
    m_current_size_bytes += embedding.size_bytes();

    // 检查是否需要淘汰
    while (needs_eviction()) {
        evict_lru();
    }

    LOG_DEBUG("EmbeddingCache put: text='{}', size={} bytes",
              text.substr(0, 50), embedding.size_bytes());
}

void EmbeddingCache::put_batch(const std::unordered_map<std::string, Embedding>& items) {
    for (const auto& [text, embedding] : items) {
        put(text, embedding);
    }
    LOG_INFO("EmbeddingCache put_batch: {} items", items.size());
}

void EmbeddingCache::remove(const std::string& text) {
    std::lock_guard lock(m_mutex);

    auto it = m_cache_map.find(text);
    if (it != m_cache_map.end()) {
        m_current_size_bytes -= it->second.second.size_bytes();
        m_lru_list.erase(it->second.first);
        m_cache_map.erase(it);

        LOG_DEBUG("EmbeddingCache remove: text='{}'", text.substr(0, 50));
    }
}

bool EmbeddingCache::contains(const std::string& text) const {
    std::lock_guard lock(m_mutex);
    return m_cache_map.find(text) != m_cache_map.end();
}

size_t EmbeddingCache::size() const {
    std::lock_guard lock(m_mutex);
    return m_cache_map.size();
}

bool EmbeddingCache::empty() const {
    std::lock_guard lock(m_mutex);
    return m_cache_map.empty();
}

// ============ 统计信息 ============

CacheStats EmbeddingCache::get_stats() const {
    std::lock_guard lock(m_mutex);

    CacheStats stats = m_stats;
    stats.current_entries = m_cache_map.size();
    stats.current_size_bytes = m_current_size_bytes;

    return stats;
}

void EmbeddingCache::reset_stats() {
    std::lock_guard lock(m_mutex);

    m_stats.hits = 0;
    m_stats.misses = 0;
    m_stats.evictions = 0;

    LOG_INFO("EmbeddingCache stats reset");
}

void EmbeddingCache::log_stats() const {
    CacheStats stats = get_stats();

    LOG_INFO("EmbeddingCache Statistics:");
    LOG_INFO("  Entries: {} / {}", stats.current_entries, m_config.max_entries);
    LOG_INFO("  Size: {:.2f} MB / {:.2f} MB",
             static_cast<double>(stats.current_size_bytes) / (1024 * 1024),
             static_cast<double>(m_config.max_size_mb));
    LOG_INFO("  Hits: {}, Misses: {}, Hit Rate: {:.2f}%",
             stats.hits, stats.misses, stats.hit_rate() * 100);
    LOG_INFO("  Evictions: {}", stats.evictions);
}

// ============ 配置 ============

void EmbeddingCache::update_config(const CacheConfig& config) {
    std::lock_guard lock(m_mutex);

    m_config = config;

    // 如果新配置更严格，可能需要淘汰
    while (needs_eviction()) {
        evict_lru();
    }

    LOG_INFO("EmbeddingCache config updated: max_entries={}, max_size_mb={}",
             m_config.max_entries, m_config.max_size_mb);
}

// ============ 私有辅助方法 ============

void EmbeddingCache::evict_lru() {
    if (m_lru_list.empty()) {
        return;
    }

    // 获取最不常用的条目（列表末尾）
    std::string lru_text = m_lru_list.back();
    m_lru_list.pop_back();

    auto it = m_cache_map.find(lru_text);
    if (it != m_cache_map.end()) {
        m_current_size_bytes -= it->second.second.size_bytes();
        m_cache_map.erase(it);

        if (m_config.enable_stats) {
            m_stats.evictions++;
        }

        LOG_DEBUG("EmbeddingCache evicted: text='{}'", lru_text.substr(0, 50));
    }
}

bool EmbeddingCache::needs_eviction() const {
    // 检查条目数限制
    if (m_cache_map.size() >= m_config.max_entries) {
        return true;
    }

    // 检查大小限制
    size_t max_size_bytes = m_config.max_size_mb * 1024 * 1024;
    if (m_current_size_bytes >= max_size_bytes) {
        return true;
    }

    return false;
}

void EmbeddingCache::update_current_size() {
    m_current_size_bytes = 0;
    for (const auto& [text, pair] : m_cache_map) {
        m_current_size_bytes += pair.second.size_bytes();
    }
}

} // namespace DearTs::Plugins::MemoryCore::RAG
