/**
 * @file embedding_cache.hpp
 * @brief 向量嵌入缓存 - LRU 缓存实现
 *
 * 功能：
 * - 缓存文本向量嵌入，避免重复计算
 * - LRU（最近最少使用）淘汰策略
 * - 支持最大缓存大小配置
 */

#pragma once

#include "core/result.h"
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <optional>
#include <mutex>

namespace DearTs::Plugins::MemoryCore::RAG {

/**
 * @brief 向量嵌入结构
 */
struct Embedding {
    std::vector<float> vector;     ///< 向量数据
    std::string model;             ///< 使用的模型名称
    int dimension;                 ///< 向量维度
    int64_t created_at;            ///< 创建时间（Unix 毫秒）
    int64_t accessed_at;           ///< 最后访问时间（Unix 毫秒）

    /**
     * @brief 计算向量大小（字节）
     */
    size_t size_bytes() const {
        return vector.size() * sizeof(float);
    }
};

/**
 * @brief LRU 缓存配置
 */
struct CacheConfig {
    size_t max_entries;            ///< 最大条目数
    size_t max_size_mb;            ///< 最大缓存大小（MB）
    bool enable_stats;             ///< 是否启用统计

    /**
     * @brief 默认配置
     */
    static CacheConfig default_config() {
        return CacheConfig{
            .max_entries = 1000,
            .max_size_mb = 100,
            .enable_stats = true
        };
    }
};

/**
 * @brief 缓存统计信息
 */
struct CacheStats {
    size_t hits;                   ///< 缓存命中次数
    size_t misses;                 ///< 缓存未命中次数
    size_t evictions;              ///< 淘汰次数
    size_t current_entries;        ///< 当前条目数
    size_t current_size_bytes;     ///< 当前缓存大小（字节）

    /**
     * @brief 计算命中率
     */
    double hit_rate() const {
        size_t total = hits + misses;
        return total > 0 ? static_cast<double>(hits) / total : 0.0;
    }
};

/**
 * @brief 向量嵌入缓存（LRU 实现）
 *
 * 提供高效的向量嵌入缓存功能
 */
class EmbeddingCache {
public:
    /**
     * @brief 获取单例实例
     */
    static EmbeddingCache& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    EmbeddingCache(const EmbeddingCache&) = delete;
    EmbeddingCache& operator=(const EmbeddingCache&) = delete;

    // ============ 初始化 ============

    /**
     * @brief 初始化缓存
     * @param config 缓存配置
     */
    void initialize(const CacheConfig& config = CacheConfig::default_config());

    /**
     * @brief 清空缓存
     */
    void clear();

    // ============ 缓存操作 ============

    /**
     * @brief 获取缓存的嵌入
     * @param text 文本内容
     * @return 嵌入向量（如果缓存命中）
     */
    std::optional<Embedding> get(const std::string& text);

    /**
     * @brief 添加嵌入到缓存
     * @param text 文本内容
     * @param embedding 嵌入向量
     */
    void put(const std::string& text, const Embedding& embedding);

    /**
     * @brief 批量添加嵌入到缓存
     * @param items 文本到嵌入的映射
     */
    void put_batch(const std::unordered_map<std::string, Embedding>& items);

    /**
     * @brief 移除缓存条目
     * @param text 文本内容
     */
    void remove(const std::string& text);

    /**
     * @brief 检查缓存是否包含指定文本
     * @param text 文本内容
     * @return 是否存在
     */
    bool contains(const std::string& text) const;

    /**
     * @brief 获取缓存大小
     * @return 当前条目数
     */
    size_t size() const;

    /**
     * @brief 检查缓存是否为空
     * @return 是否为空
     */
    bool empty() const;

    // ============ 统计信息 ============

    /**
     * @brief 获取缓存统计信息
     * @return 统计信息
     */
    CacheStats get_stats() const;

    /**
     * @brief 重置统计信息
     */
    void reset_stats();

    /**
     * @brief 打印统计信息到日志
     */
    void log_stats() const;

    // ============ 配置 ============

    /**
     * @brief 获取当前配置
     * @return 缓存配置
     */
    const CacheConfig& get_config() const { return m_config; }

    /**
     * @brief 更新配置
     * @param config 新配置
     */
    void update_config(const CacheConfig& config);

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    EmbeddingCache() = default;

    /**
     * @brief 析构函数
     */
    ~EmbeddingCache() = default;

    /**
     * @brief LRU 淘汰最旧的条目
     */
    void evict_lru();

    /**
     * @brief 检查是否需要淘汰
     * @return 是否需要淘汰
     */
    bool needs_eviction() const;

    /**
     * @brief 计算缓存当前大小（字节）
     */
    void update_current_size();

    // ============ 成员变量 ============

    // LRU 缓存实现：list 存储键的访问顺序，map 存储键到值的映射
    using ListType = std::list<std::string>;
    using MapType = std::unordered_map<std::string, std::pair<ListType::iterator, Embedding>>;

    ListType m_lru_list;                      ///< LRU 列表（最常用在前）
    MapType m_cache_map;                      ///< 缓存映射
    mutable std::mutex m_mutex;               ///< 线程安全互斥锁

    CacheConfig m_config;                     ///< 缓存配置
    CacheStats m_stats;                       ///< 统计信息
    size_t m_current_size_bytes;             ///< 当前缓存大小（字节）
};

} // namespace DearTs::Plugins::MemoryCore::RAG
